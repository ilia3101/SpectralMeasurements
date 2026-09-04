import argparse
import pickle
import datetime
import random
import time
from time import gmtime, strftime
from enum import IntEnum
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Any, Callable, List, Optional, Generator
import pigpio
import signal
from fractions import Fraction

class DisableCtrlC:
    _count = 0
    _previous_handler = None
    def __enter__(self):
        cls = type(self)
        if cls._count == 0:
            cls._previous_handler = signal.getsignal(signal.SIGINT)
            signal.signal(signal.SIGINT, signal.SIG_IGN)
        cls._count += 1
        return self
    def __exit__(self, exc_type, exc, tb):
        cls = type(self)
        cls._count -= 1
        if cls._count == 0:
            signal.signal(signal.SIGINT, cls._previous_handler)
            cls._previous_handler = None
        return False



def plot_line(x, y, filename="plot_new.png"):
    import matplotlib.pyplot as plt
    print("Saving plot...")
    plt.plot(x, y)
    plt.savefig(filename)
    plt.close()


# def set_var(name: str | Path, obj: Any) -> None:
#     with open(PERSISTENT_STORAGE_PATH + "/" + name, "wb") as f:
#         pickle.dump(obj, f)

# def get_var(name: str, ) -> Optional[Any]:
#     path = Path(PERSISTENT_STORAGE_PATH + "/" + name)
#     if path.exists:
#         with open(path, "rb") as f:
#             return pickle.load(f)
#     else:
#         return None


def send_pulses(
    pi, pin: int, num_pulses: int, period_us: int, pulse_duration_us: int = 10
) -> None:
    pi.set_mode(pin, pigpio.OUTPUT)

    BATCH_SIZE = 2000

    pulses = []
    for i in range(0, BATCH_SIZE):
        pulses.append(pigpio.pulse(1 << pin, 0, pulse_duration_us))
        pulses.append(pigpio.pulse(0, 1 << pin, period_us - pulse_duration_us))

    pulses_remaining = (
        int(num_pulses) * 2
    )  # multiplied by two because each pulse is actually 2 steps in a wave

    while pulses_remaining != 0:
        if len(pulses) > pulses_remaining:
            pulses = pulses[:pulses_remaining]
        pi.wave_clear()
        pi.wave_add_generic(pulses)
        wave_id = pi.wave_create()
        pi.wave_send_once(wave_id)
        while pi.wave_tx_busy():
            pass
        pi.wave_delete(wave_id)
        pulses_remaining -= len(pulses)


def set_servo_position(pi, pin: int, pos: float):
    duration_us = int(pos * 2000 + 500.5)
    send_pulses(pi, pin, 1, 2500, duration_us)


def take_measurements(
    start_wl: int,
    end_wl: int,
    wl_step: int,
    set_wl: Callable[[int], None],
    take_reading: Callable[[], int],
) -> None:
    readings = []
    wl = start_wl
    while wl <= end_wl:
        set_wl(wl)
        print(f"Wavelength set to {wl}")
        reading = take_reading()
        readings.append(reading)
        wl += wl_step


def set_pin(pi, pin: int, level: int) -> None:
    pi.set_mode(pin, pigpio.OUTPUT)
    pi.write(pin, level)


def measure_pulses(pi, pin: int, duration: float) -> int:
    pi.set_mode(pin, pigpio.INPUT)
    cb = pi.callback(pin, pigpio.RISING_EDGE)
    try:
        cb.reset_tally()
        time.sleep(duration)
        return cb.tally()
    finally:
        cb.cancel()


def step_stepper(
    pi,
    step_dir: tuple[int, int],
    steps: int,
    steps_per_second: int = 40,
    substeps: int = 1,
) -> None:
    step_period_us = int(1_000_000 / (steps_per_second * substeps) + 0.5)
    set_pin(pi, step_dir[1], 1 if steps > 0 else 0)
    send_pulses(
        pi,
        step_dir[0],
        abs(steps * substeps),
        step_period_us,
        pulse_duration_us=5,
    )



# Step position saving...
class StateSave:
    @staticmethod
    def set_var(key: str, value: int):
        Path(str(Path.home()) + "/" + ".spectral_variable_" + key).write_text(str(value), encoding="utf-8")

    @staticmethod
    def get_var(key: str) -> int | None:
        try:
            return int(Path(str(Path.home()) + "/" + ".spectral_variable_" + key).read_text(encoding="utf-8"))
        except FileNotFoundError:
            return None

    @staticmethod
    def get_var_or_set_default(key: str, default: int) -> int:
        value = StateSave.get_var(key)
        if value is not None:
            return value
        else:
            StateSave.set_var(key, default)
            return default


@dataclass
class WlParams:
    base_step: int = field(default=0)
    base_step_wl: float = field(default=632.8)
    wl_per_steps: tuple[float, int] = field(default=(25.0, -3200))

    def __init__(self):
        self.base_step_wl = StateSave.get_var_or_set_default("wl_at_zero_step", 632800) / 1000.0
        self.wl_per_steps = (
            StateSave.get_var_or_set_default("revolution_wl", 25000) / 1000.0,
            StateSave.get_var_or_set_default("revolution_steps", -3200)
        )

    def get_wl_for_step(self, step: int) -> float:
        return (
            self.base_step_wl
            + float(step) / float(self.wl_per_steps[1]) * self.wl_per_steps[0]
        )

    def get_step_for_wl(self, wl: float) -> int:
        return int(round(
            (wl - self.base_step_wl)
            / self.wl_per_steps[0]
            * float(self.wl_per_steps[1])
        ))


# TODO: do backlash decision logic by accumulating movement in current direction and seeing if already 'done' maybe? maybe this is too complex
class WlControl:
    def __init__(self, pi, step_dir: tuple[int, int] = (27, 22)):
        # self.is_first = True
        self.pi: Any = pi
        self.step_dir: tuple[int, int] = step_dir
        self.is_first: bool = True
        self.current_step: int = StateSave.get_var_or_set_default("current_step", 0)
        self.backlash_steps: int = StateSave.get_var_or_set_default("backlash_steps", -4000)  # can be positive or negative
        # print(f"backlash steps = {self.backlash_steps}")
        self.steps_per_second: int = 1500
        self.step_range: tuple[int, int] = (-1_000_000_000, 1_000_000_000)
        self.wlparams: WlParams = WlParams()

    def set_wl(self, wl: float, do_backlash: bool = True) -> None:
        with DisableCtrlC():
            final_pos = self.wlparams.get_step_for_wl(wl)
            # print(f"New step = {final_pos} (curr = {self.current_step}), diff = {final_pos - self.current_step}")
            move_steps = final_pos - self.current_step
            backlash_sign = self.backlash_steps < 0
            move_sign = move_steps < 0
            if move_sign == backlash_sign:
                do_backlash = False
            if do_backlash or self.is_first:
                intermediate_pos = final_pos - self.backlash_steps
                self._go_to_step(self._clip_to_range(intermediate_pos))
                self._go_to_step(self._clip_to_range(final_pos))
            else:
                self._go_to_step(self._clip_to_range(final_pos))
            self.is_first = False

    def _clip_to_range(self, step: int) -> int:
        return max(min(self.step_range[1], step), self.step_range[0])

    def _go_to_step(self, step: int) -> None:
        with DisableCtrlC():
            steps = step - self.current_step
            speed = self.steps_per_second
            if abs(steps) < 1000:
                speed = 200
            # print(f"Moving {steps} steps")
            step_stepper(
                self.pi,
                self.step_dir,
                steps,
                self.steps_per_second,
            )
            self.current_step = step
            StateSave.set_var("current_step", self.current_step)


# def measure_sweep(
#     wlc: WlControl,
#     read_cb: Callable[[], Any],
#     from_wl: float,
#     to_wl: float,
#     steps: int,
# ) -> Generator[tuple[float, int, Any], None, None]:
#     """
#     Returns: Tuple(Wavelengths, Step positions, Readings from read_cb)
#     """
#     # wls = []
#     # step_values = []
#     # readings = []
#     for i in range(0, steps):
#         wl = from_wl + (to_wl - from_wl) * (i / (steps - 1))
#         wlc.set_wl(wl)
#         # wls.append(wl)
#         # step_values.append(wlc.current_step)
#         reading = read_cb()
#         # readings.append(reading)

#         yield (wl, wlc.current_step, reading)

#         print(wl)
#         print(reading)

#     # return (wls, step_values, readings)

# class syntax
class FilterOption(IntEnum):
    NO_FILTER = 0
    VIOLET_375_425 = 3
    DEEP_RED = 2

# Position options 0, 1, 2, 3
def set_filter_wheel(pi, pos: FilterOption):
    POS_FIRST_BACKLASH_POS = 0.455
    POS_FIRST = 0.473
    POS_FINAL = 0.893

    position = POS_FIRST + ((POS_FINAL - POS_FIRST) / 3) * int(pos)
    start_time = time.perf_counter()

    # ALl the extra movement is to combat friction/backlash issues
    for i in range(0,20):
        set_servo_position(pi, 24, POS_FIRST_BACKLASH_POS)
        time.sleep(0.01)
    time.sleep(0.05)

    current_position = POS_FIRST
    speed_factor_per_second = 0.4

    while current_position < position:
        elapsed = time.perf_counter() - start_time
        current_position = POS_FIRST + elapsed * speed_factor_per_second
        set_servo_position(pi, 24, current_position)
        time.sleep(0.005)

    for i in range(0,10):
        set_servo_position(pi, 24, position)
        time.sleep(0.01)

def set_light_cover(pi, closed: bool):
    servopos = 1.0 if closed else 0.0
    for i in range(0,20):
        set_servo_position(pi, 25, servopos)
        time.sleep(0.01)
    time.sleep(0.3)

def activate_camera(pi):
    set_pin(pi, 14, 0)
    time.sleep(0.15)
    set_pin(pi, 14, 1)



def measure_camera():
    pi = pigpio.pi()

    # Light cover test
    set_light_cover(pi, True)
    set_light_cover(pi, False)
    set_light_cover(pi, True)

    # Camera test
    set_pin(pi, 14, 0)
    time.sleep(0.1)
    set_pin(pi, 14, 1)
    activate_camera(pi)

    # Filter test
    set_filter_wheel(pi, FilterOption.NO_FILTER)
    time.sleep(0.5)
    set_filter_wheel(pi, FilterOption.VIOLET_375_425)
    time.sleep(0.5)
    set_filter_wheel(pi, FilterOption.DEEP_RED)
    time.sleep(0.5)
    set_filter_wheel(pi, FilterOption.NO_FILTER)
    time.sleep(0.5)

    for i in range(0,2):
        diode_value = measure_pulses(pi, 4, 1.75)
        print(f"Diode = {diode_value}")

    # Servo pulse pin
    SERVO_PIN = 24

    wl_control = WlControl(pi)

    start_wl = 380
    wl_step = 2.0
    max_wl = 720

    diode_exposure_time = 3.75

    current_wl = start_wl

    # Track filter state. TODO: do this nicer in future
    passed_violet_threshold = False
    violet_threshold = 420
    passed_red_threshold = False
    red_threshold = 650

    set_filter_wheel(pi, FilterOption.VIOLET_375_425)

    while current_wl <= max_wl:
        print(f"current wl = {current_wl}")
        wl_control.set_wl(current_wl)

        # Change the filter wheel when needed
        if current_wl > violet_threshold and not passed_violet_threshold:
            set_filter_wheel(pi, FilterOption.NO_FILTER)
            passed_violet_threshold = True
        if current_wl >= red_threshold and not passed_red_threshold:
            set_filter_wheel(pi, FilterOption.DEEP_RED)
            passed_red_threshold = True

        # Uncover the light
        set_light_cover(pi, False)

        # activate camera and measure pulses (light)
        activate_camera(pi)
        diode_value_light = measure_pulses(pi, 4, diode_exposure_time)
        print(f"{current_wl}, {strftime('%Y-%m-%d %H:%M:%S', gmtime())}, LIGHT, {diode_value_light}")

        # Cover the light, take a dark frame
        set_light_cover(pi, True)

        # Camera writing break
        time.sleep(0.6)

        # activate camera and measure pulses (dark)
        activate_camera(pi)
        diode_value_dark = measure_pulses(pi, 4, diode_exposure_time)

        print(f"{current_wl}, {strftime('%Y-%m-%d %H:%M:%S', gmtime())}, DARK, {diode_value_dark}")

        # Cool down break
        time.sleep(0.6)

        current_wl += wl_step

    wl_control.set_wl(632.8)


def set_wl():
    import sys
    wavelength = float(sys.argv[1])
    print(f"Setting wavelength to {wavelength}nm")

    pi = pigpio.pi()
    wl_control = WlControl(pi)

    wl_control.set_wl(wavelength)

def uncover_light():
    print("Uncovering light")
    pi = pigpio.pi()
    set_light_cover(pi, False)

def cover_light():
    print("Covering light")
    pi = pigpio.pi()
    set_light_cover(pi, True)

def measure_sweep():
    import tqdm
    import sys

    if len(sys.argv) != 5:
        print("usage: measure_sweep <wl_start> <wl_end> <samples> <sampling_time>\nFor example: uv run measure_sweep 630 634 500 0.1")
    wl_start = float(sys.argv[1])
    wl_end = float(sys.argv[2])
    samples = int(sys.argv[3])
    sampling_time = float(sys.argv[4])

    pi = pigpio.pi()
    wl_control = WlControl(pi)

    x = [0.0 for i in range(0,samples)]
    y = [0.0 for i in range(0,samples)]

    for m in range(0,30):
        print("Measuring sweep...")
        for i in tqdm.tqdm(range(0,samples)):
        # for i in range(0,samples):
            wl = wl_start + (wl_end - wl_start) * (i/(samples-1))
            wl_control.set_wl(wl)
            x[i] = wl
            pulses = measure_pulses(pi, 4, sampling_time)
            y[i] += pulses
        plot_line(x, y)

    max_value = y[0]
    max_value_wl = x[0]
    for i in range(0,len(y)):
        if y[i] > max_value:
            max_value = y[i]
            max_value_wl = x[i]

    print(f"Max value at {max_value_wl:.2f}nm")

    print("Making plot with matplotlib")
    # import matplotlib.pyplot as plt
    # plt.plot(x, y)
    # plt.savefig("poop.png")
    # plt.close()


def set_filter():
    import sys
    option = sys.argv[1]

    filters = {
        "none": FilterOption.NO_FILTER,
        "red": FilterOption.DEEP_RED,
        "violet": FilterOption.VIOLET_375_425
    }

    if option not in filters:
        print("Filter options are \"none\", \"red\", and \"violet\"")
    else:
        print(f"Setting filter to {option}")
        pi = pigpio.pi()
        set_filter_wheel(pi, filters[option])

def set_variable():
    import sys
    key = sys.argv[1]
    value = int(sys.argv[2])
    StateSave.set_var(key, value)
