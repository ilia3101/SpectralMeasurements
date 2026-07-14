import argparse
import pickle
import random
import time
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Any, Callable, List, Optional, Tuple

import matplotlib.pyplot as plt
import pigpio

PERSISTENT_STORAGE_PATH: str = "/home/ilia"


def plot_line(x, y, filename="plot.png"):
    plt.plot(x, y)
    plt.savefig(filename)
    plt.close()


# def set_var(name: str | Path, obj: Any) -> None:
#     with open(PERSISTENT_STORAGE_PATH + "/" + name, "wb") as f:
#         pickle.dump(obj, f)


def get_var(name: str) -> Optional[Any]:
    path = Path(PERSISTENT_STORAGE_PATH + "/" + name)
    if path.exists:
        with open(path, "rb") as f:
            return pickle.load(f)
    else:
        return None


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
    send_pulses(pi, pin, 1, 20000, duration_us)


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
    step_dir: Tuple[int, int],
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


@dataclass
class WlParams:
    base_step: int = field(default=0)
    base_step_wl: float = field(default=632.8)
    wl_per_steps: Tuple[float, int] = field(default=(25.0, -3200))

    def get_wl_for_step(self, step: int) -> float:
        return (
            self.base_step_wl
            + float(step) / float(self.wl_per_steps[1]) * self.wl_per_steps[0]
        )

    def get_step_for_wl(self, wl: float) -> int:
        return int(
            (wl - self.base_step_wl)
            / self.wl_per_steps[0]
            * float(self.wl_per_steps[1])
            + 0.5
        )


# TODO: do backlash decision logic by accumulating movement in current direction and seeing if already 'done' maybe? maybe this is too complex
class WlControl:
    def __init__(self, pi, step_dir: Tuple[int, int]):
        # self.is_first = True
        self.pi = pi
        self.step_dir = step_dir
        self.is_first = True
        self.current_step = 0
        self.backlash_steps = -400  # can be positive or negative
        self.steps_per_second = 1500
        self.step_range = (-1_000_000_000, 1_000_000_000)
        self.wlparams = WlParams()

    def set_wl(self, wl: float, do_backlash: bool = True) -> None:
        final_pos = self.wlparams.get_step_for_wl(wl)
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
        steps = step - self.current_step
        print(f"Moving {steps} steps")
        step_stepper(
            self.pi,
            self.step_dir,
            steps,
            self.steps_per_second,
        )
        self.current_step = step


def measure_sweep(
    wlc: WlControl,
    read_cb: Callable[[], Any],
    from_wl: float,
    to_wl: float,
    steps: int,
) -> Tuple[List[float], List[int], List[Any]]:
    """
    Returns: Tuple(Wavelengths, Step positions, Readings from read_cb)
    """
    wls = []
    step_values = []
    readings = []
    for i in range(0, steps):
        wl = from_wl + (to_wl - from_wl) * (i / (steps - 1))
        wlc.set_wl(wl)
        wls.append(wl)
        step_values.append(wlc.current_step)
        readings.append(read_cb())

        print(wl)
        print(read_cb())

    return (wls, step_values, readings)


def main():
    pi = pigpio.pi()

    # (step pin, direction pin)
    STEPPER_PINS = (27, 22)

    # Servo pulse pin
    SERVO_PIN = 24

    wl_control = WlControl(pi, STEPPER_PINS)
    # wl_control.set_wl(530)
    # time.sleep(25)
    # wl_control.set_wl(632.8)
    #

    widen = 0
    wls, step_pos_list, readings = measure_sweep(
        wl_control, lambda: measure_pulses(pi, 4, 0.3), 629 - widen, 635 + widen, 17
    )

    wl_control.set_wl(632.8)

    plot_line(wls, readings)

    print("Moving Servo")
    for i in range(0, 10):
        set_servo_position(pi, SERVO_PIN, random.random())
        time.sleep(0.35)

    for i in range(0, 5):
        print(measure_pulses(pi, 4, 0.1))
        time.sleep(0.05)

    # take_measurements(400, 700, 10, lambda a: None, lambda: 69)


if __name__ == "__main__":
    main()
