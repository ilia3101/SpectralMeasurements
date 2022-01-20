""" 
Processes data from ISO TR folder and outputs as normalised 380-750nm
(extrapolated, interpolated etc)

/usr/local/Cellar/python@3.9/3.9.6/bin/python3 prepare_reflectance_data.py
"""

import colour
import os

iso_tr_data_path = 'C037358e Electronic inserts/'
output_folder = 'ReflectanceISO/'

if not os.path.exists(output_folder):
    os.mkdir(output_folder)

extensions = ['org', 'int']

calibration_path = 'SourceData/org/calib'

include = [
    # I've put a low weight on skin cause there's sooo many samples
    'SourceData/org/face/kao', 0.2,
    'SourceData/org/face/kawasaki', 0.2,
    'SourceData/org/face/ooka', 0.2,
    'SourceData/org/face/oulu', 0.2,
    'SourceData/org/face/shiseido', 0.2,
    'SourceData/org/face/sun', 0.2,

    'SourceData/org/flowers/', 0.7,
    'SourceData/org/graphic/', 0.05,
    'SourceData/org/leaves/', 1.3,
    'SourceData/org/paint/', 0.1,
    'SourceData/org/paints/', 0.1,
    'SourceData/org/photo/', 0.02,
    'SourceData/org/textiles/', 0.4,

    'SourceData/int/krinov/', 0.3,

    'SourceData/int/krinov/', 0.35 # We will use pre processed krinov data, as it's hard to process form scratch
]

""" 
Will output an array with data and INSTRUMENTATION number (for matching to calibration)
"""
def load_data_file(path, get_instrument):
    with open(path, 'r') as file:
        data = file.read().split('SPECTRUM')
    data_items = []

    for spectrum in data:
        item = spectrum.split('\nBEGIN_DATA\n')
        if (len(item) > 1):
            spectral_data = (item[1].split("\nEND_DATA\n"))[0].split("\n")
            spectral_data_dict = {} #fucking stupid colour way of initialising
            for i in range(0,len(spectral_data)):
                dict_index = int(float(spectral_data[i].split()[0]))
                spectral_data_dict[dict_index] = float(spectral_data[i].split()[1])
                spectral_data[i] = [float(spectral_data[i].split()[0]), float(spectral_data[i].split()[1])]

            spectral_data_as_SpectralDistribution = colour.SpectralDistribution(spectral_data_dict)

            #instrument number
            if (get_instrument):
                instrumentation = int((spectrum.split("\nINSTRUMENTATION"))[1].split("\"")[1].split(":")[0])
            else:
                instrumentation = -1

            #more like "NAME" not "LOC"
            sample_name = (spectrum.split("\nSAMPLE_LOC"))[1].split("\"")[1]

            data_tuple = (sample_name, spectral_data_as_SpectralDistribution, instrumentation)

            data_items.append(data_tuple)

    return data_items

#wait we don't need this? but I already wrote the code to load this stuff
calibration_spectra = {}

# taken from Readme-INT.pdf
normalisation_factors = {
    1:0.9929,
    2:0.9924,
    3:1.0305,
    4:1.0450,
    5:1.0372,
    6:1.0004,
    7:1.0123,
    8:1.0005,
    9:1.0051,
    10:1.0014,
    11:1.0191,
    12:1.0140,
    13:0.9856,
    14:1.0209,
    15:1.0249,
    16:1.0197
}

def load_calibration_spectra():
    files = os.listdir(iso_tr_data_path + '/' + calibration_path)
    for filename in files:
        if (len(filename) > 0 and filename[0] != '.' and '.' in filename and filename.split('.')[1] == 'org'):
                data = load_data_file(iso_tr_data_path + '/' + calibration_path + '/' + filename, True)
                white_spectrum = data[1]
                print("Calibration found: '" + white_spectrum[0] + "', instrument = " + str(white_spectrum[2]))
                calibration_spectra[white_spectrum[2]] = white_spectrum

def do_calibration(data_array):
    data_processed = []
    for spectrum in data_array:
        instrument_id = spectrum[2]
        spectral_distribution_processed = spectrum[1] * normalisation_factors[instrument_id]
        data_processed.append((spectrum[0], spectral_distribution_processed, spectrum[2]))
    return data_processed

def reshape_spectra(data_array):
    reshaped = []
    for spectrum in data_array:
        spectrum = spectrum[1].extrapolate(colour.SpectralShape(380,750))
        reshaped.append(spectrum.interpolate(colour.SpectralShape(interval=2)))
    return reshaped


load_calibration_spectra()

data = []

#Load all data from ORG
for path in include:
    print(path)
    if isinstance(path, str):
        files = os.listdir(iso_tr_data_path + '/' + path)
        for filename in files:
            if (len(filename) > 0 and filename[0] != '.' and '.' in filename and (filename.split('.')[1] == 'org' or filename.split('.')[1] == 'int')):
                # Exlude transparency spectra I only want reflectance
                if ('_t' not in filename):
                    fullpath = iso_tr_data_path + '/' + path + '/' + filename
                    print("Loading data file: " + fullpath)
                    if (filename.split('.')[1] == 'int'):
                        data = data + load_data_file(fullpath, False) # int files dont need clibration
                    else:
                        data = data + do_calibration(load_data_file(fullpath, True))
    
    print(str(len(data)) + " Data items loaded!")

print("Reshaping, interpolating/extrapolating spectra...")
data = reshape_spectra(data)

print("Saving spectra...")

count = 0
num_below_zero = 0
for spectrum in data:
    filename = str(count).rjust(6,'0') + ".dat"
    file = open(output_folder + '/' + filename, "w")
    start = spectrum.shape.start
    interval = spectrum.shape.interval
    for i in range (0, len(spectrum.values)):
        spectral_value = spectrum.values[i]
        wl = start + interval*i
        if (spectral_value < 0):
            num_below_zero = num_below_zero + 1
            print("Value" + str(spectral_value) + "found at " + str(wl) + "nm, clamping to zero. In file: " + filename)
            spectral_value = 0.0
        file.write(str(wl) + " " + str(spectral_value) + "\n")
    file.close()
    count = count + 1

print("Below zero values encounterd this many times: " + str(num_below_zero))