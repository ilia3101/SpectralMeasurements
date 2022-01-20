import colour
import os

output_folder = 'EmissionGenerated/'

if not os.path.exists(output_folder):
    os.mkdir(output_folder)

custom_cmfs = (
    colour.colorimetry.MSDS_CMFS_STANDARD_OBSERVER['CIE 2012 10 Degree Standard Observer'].copy().align(colour.SpectralShape(390, 710, 10))
)

limited_cmfs = (
    colour.colorimetry.MSDS_CMFS_STANDARD_OBSERVER['CIE 2012 10 Degree Standard Observer'].copy().align(colour.SpectralShape(424, 670, 5))
)

dimension = 5

num_spectra = 0

cmf_x, cmf_y, cmf_z = limited_cmfs.to_sds()

min_distance = 0.5

data = []

# for i in range (0, len(cmf_x.values), 7):
#     distance = 0.96
#     dist_reduction = 0.6
#     dist_reduction_reduction = 0.98
#     print("starting wavelength" + str(i * cmf_x.shape.interval + cmf_x.shape.start))
#     while distance > 0.2:
#         X = cmf_x.values[i]
#         Y = cmf_y.values[i]
#         Z = cmf_z.values[i]
#         print("HEllo")

#         X = X*distance + Y*(1.0-distance)
#         Z = Z*distance + Y*(1.0-distance)

#         distance *= dist_reduction
#         dist_reduction *= dist_reduction_reduction

#         sd = colour.recovery.XYZ_to_sd_Meng2015([X,Y,Z], cmfs=custom_cmfs)
#         data.append(sd)

dimension = 150
for v in range (0, dimension, 7):
    for u in range (0, dimension, 7):
        in_XYZ = colour.xy_to_XYZ(colour.Luv_uv_to_xy([u/dimension*0.65,v/dimension*0.65]))
        in_XYZ = in_XYZ# / 100.0
        if (colour.is_within_visible_spectrum(in_XYZ, cmfs=custom_cmfs)):
            print(in_XYZ)
            sd = colour.recovery.XYZ_to_sd_Meng2015(in_XYZ, cmfs=custom_cmfs)
        # print("starting wavelength" + str(i * cmf_x.shape.interval + cmf_x.shape.start))
        # while distance > 0.2:
        #     X = cmf_x.values[i]
        #     Y = cmf_y.values[i]
        #     Z = cmf_z.values[i]
        #     print("HEllo")

        #     X = X*distance + Y*(1.0-distance)
        #     Z = Z*distance + Y*(1.0-distance)

        #     distance *= dist_reduction
        #     dist_reduction *= dist_reduction_reduction

        #     sd = colour.recovery.XYZ_to_sd_Meng2015([X,Y,Z], cmfs=custom_cmfs)
        #     data.append(sd)
 
print("Writing to file")

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

# for y in range (0,dimension):
#     v_coord = y / 0.65 + 0.001
#     for x in range (0,dimension):
#         u_coord = x / 0.65 + 0.001
#         y=0.01
#         XYZ = [y*(9*u_coord)/(4*v_coord), y, y*(12-3*u_coord-20*v_coord)/(4*v_coord)]
#         if (colour.is_within_visible_spectrum(XYZ, cmfs=custom_cmfs)):
#             sd = colour.recovery.XYZ_to_sd_Meng2015(XYZ, cmfs=custom_cmfs)
#         else:
#             sd = 0