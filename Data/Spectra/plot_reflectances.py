
import numpy as np
from scipy.spatial import ConvexHull, convex_hull_plot_2d
import colour
import os
import matplotlib.pyplot as plt
import matplotlib


directories = [
    # 'ReflectanceISO/',
    # 'EmissionGenerated/',
    'GaussianGenerated/',
]

# directory = 'EmissionGenerated/'
# directory = 'GaussianGenerated/'

spectral_distributions = []

for directory in directories:
    files = os.listdir(directory)

    files = files[1::4]
    for filename in files:
        if (len(filename) > 0 and filename[0] != '.' and '.' in filename and (filename.split('.')[1] == 'dat')):
            datapoint_as_dict = {}
            datapoint_as_dict2 = {}
            print(filename)
            with open(directory + '/' + filename, 'r') as file:
                data = file.read().split('\n')
            for d in data:
                d = d.split()
                if (len(d) == 2):
                    datapoint_as_dict[int(float(d[0]))] = float(d[1])
                    if (directory == 'ReflectanceISO/'): datapoint_as_dict2[int(float(d[0]))] = float(d[1])*float(d[1])
            if (len(datapoint_as_dict) > 10):
                spectral_distributions.append(colour.SpectralDistribution(datapoint_as_dict).align(colour.SpectralShape(380, 750, 2)))
                if (directory == 'ReflectanceISO/'): spectral_distributions.append(colour.SpectralDistribution(datapoint_as_dict2).align(colour.SpectralShape(380, 750, 2)))
            # if (len(spectral_distributions) > 2000): break

our_cmf = colour.colorimetry.MSDS_CMFS_STANDARD_OBSERVER['CIE 2012 10 Degree Standard Observer'].align(colour.SpectralShape(380, 750, 2))


""" 
Opotional plot as camera mode
"""
plot_as_camera = False
camera_response_path = "/Volumes/SPECTRAL/Data/Camera/Canon/EOS 5D Mark III/001/response.dat"
M = np.array([
    [ 0.6722, -0.0635, -0.0963],
    [-0.4287,  1.2460,  0.2028],
    [-0.0908,  0.2162,  0.5668]
])
M = np.linalg.inv(M)
if (plot_as_camera):
    with open(camera_response_path, 'r') as file:
        data = file.read().split('\n')
    cam_XYZ = {}
    for line in data:
        response = line.split()
        if (len(response) > 3):
            wl = int(float(response[0]))
            cam_r = float(response[1])
            cam_g = float(response[2])
            cam_b = float(response[3])
            cam_XYZ[wl] = (cam_r*M[0][0] + cam_g*M[0][1] + cam_b*M[0][2],
                           cam_r*M[1][0] + cam_g*M[1][1] + cam_b*M[1][2],
                           cam_r*M[2][0] + cam_g*M[2][1] + cam_b*M[2][2])
    our_cmf = colour.colorimetry.XYZ_ColourMatchingFunctions(cam_XYZ).align(colour.SpectralShape(380, 750, 2))


plot = 'uv'
# plot = 'xy'
# plot = 'IPT' #this one doesnt work well




x_points = []
y_points = []

for s in spectral_distributions:
    X,Y,Z = colour.colorimetry.sd_to_XYZ_integration(s, cmfs=our_cmf)
    if (plot == 'uv'):
        x_points.append((4*X)/(X+15*Y+3*Z))
        y_points.append((9*Y)/(X+15*Y+3*Z))
    elif (plot == 'xy'):
        x_points.append(X/(X+Y+Z))
        y_points.append(Y/(X+Y+Z))
    elif (plot == 'IPT'):
        x_points.append(colour.XYZ_to_IPT([X,Y,Z]/Y)[1])
        y_points.append(colour.XYZ_to_IPT([X,Y,Z]/Y)[2])

locus_x = []
locus_y = []
cmfX,cmfY,cmfZ = our_cmf.align(colour.SpectralShape(410, 690, 5)).to_sds()

for i in range (0,len(cmfX.values)):
    X = cmfX.values[i]
    Y = cmfY.values[i]
    Z = cmfZ.values[i]
    if (plot == 'uv'):
        locus_x.append((4*X)/(X+15*Y+3*Z))
        locus_y.append((9*Y)/(X+15*Y+3*Z))
    elif (plot == 'xy'):
        locus_x.append(X/(X+Y+Z))
        locus_y.append(Y/(X+Y+Z))
    elif (plot == 'IPT'):
        locus_x.append(colour.XYZ_to_IPT([X,Y,Z]/Y)[1])
        locus_y.append(colour.XYZ_to_IPT([X,Y,Z]/Y)[2])

#locus hull
locus_as_points = np.empty((len(locus_x), 2))
for i in range (0,len(locus_x)):
    locus_as_points[i][0] = locus_x[i]
    locus_as_points[i][1] = locus_y[i]
hull = ConvexHull(locus_as_points)
convex_hull_plot_2d(hull)

if (plot == 'uv'):
    plt.ylim((-0.45, 0.69))
    plt.xlim((-0.12, 0.69))
elif (plot == 'xy'):
    plt.ylim((-0.12, 1.05))
    plt.xlim((0.0, 0.8))
elif (plot == 'IPT'):
    plt.ylim((-5, 5))
    plt.xlim((-5, 5))

plt.scatter(locus_x, locus_y, s=0.6)
plt.scatter(x_points, y_points, s=1)
plt.show()