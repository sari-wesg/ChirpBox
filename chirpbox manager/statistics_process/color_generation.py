import matplotlib
import seaborn as sns

import colorsys

def scale_lightness(rgb, scale_l):
    # convert rgb to hls
    h, l, s = colorsys.rgb_to_hls(*rgb)
    # manipulate h, l, s values and return as rgb
    return colorsys.hls_to_rgb(h, min(1, l * scale_l), s = s)

color = matplotlib.colors.ColorConverter.to_rgb("navy")
rgbs = [scale_lightness(color, scale) for scale in [0, .5, 1, 1.5, 2]]
sns.palplot(rgbs)
print(rgbs)