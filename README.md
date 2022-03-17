# gfont-png-svg
Reads Google Fonts fonts and render it to PNG and SVG

# Problem
The Editor is taking around 10s to load in a 100Mb internet because of the amount of fonts we are loading at startup.

The font loading can be divided into two types: fonts for the `GoogleFontPicker` component and fonts for the page content.

# GoogleFontPicker fonts

In this case each font in the list has to be rendered with the name of the font and the regular font-face for that font. See image below:
![image](https://user-images.githubusercontent.com/5407363/152974031-bd49bfbc-1b83-420d-a739-4defa537c9f1.png)

- We don't need to load all glyphs because we only need glyphs contained in the font name.
- Only load this fonts when the `GoogleFontPicker` is open.
- The list should lazyload fonts as the user scrolls.

# Optimization Strategies

Three options were considered for this: PNG (Compressed), SVG + Gzip and Webfontloader.
For PNG and SVG I wrote a utility program to read data from the `.ttf` font file and make the rendering.
- Github project for the utility: https://github.com/vzaramel/gfont-png-svg
- Used libs: 
  - `stb_truetype.h`: to load .ttf files
  - `lode_png`: to generate and compress raw image into a png file
  - `[font_to_svg2.hpp](https://github.com/donbright/font_to_svg/blob/master/font_to_svg2.hpp)`: Used as reference to translate from stb_truetpye extratected data and SVG paths.

# 1. **Render a PNG (Compressed) with the font name** 
 This strategy was considered because text uses a lot of data to represent the glyphs using vectors. So with a highly compressed PNG with only two channels (gray + alpha) we could get a smaller size than loading the fonts.

- The donwside of this approach is that the fonts would be rendered with a specific height and could not be scaled at runtime as we do with vector glyphs.
- As the amount of fonts increasses in the PNG the compression rate gets better.
- One advantage here is that we could build a big atalas PNG with many fonts in it and render a section of the atlas in each card.
- For the compression it was brute forced the best compression settings for each font.

## Test 1: Roboto
![Roboto](https://user-images.githubusercontent.com/5407363/152979607-36ff233b-b075-4276-a396-82a94531cba1.png)
Dimensions: 280 x 60
File Size: 2.04KB

## Test 2: Architects Daughter
![ArchitectsDaughter](https://user-images.githubusercontent.com/5407363/152986127-3c468202-7856-4b1f-a5ed-107c8a228292.png)
Dimensions: 420 x 60
File Size: 4.73KB

## Test 3: Bebas Neue
![BebasNeue](https://user-images.githubusercontent.com/5407363/152987634-ee05eb20-abe0-44a2-9ebd-bff936f17ec9.png)
Dimensions: 200x 60
File Size: 1.76KB

# 2. **Render a HTML SVG with glyphs translated to paths + gzip**
- There still possibility to improve this translation with further optimization like detecting circles and use SVG circle instead of path. But further improvements would take too much time and would not payoff.
- Read curves data from ttf file and translate it to SVG Path
Example for Roboto:
```
<svg xmlns='http://www.w3.org/2000/svg' version='1.1'><g fill-rule='nonzero' transform='translate(0, 80) scale(0.05)'><path d=' M249,-1456 L731,-1456 Q895,-1456 1008,-1406 Q1122,-1356 1181,-1258 Q1241,-1161 1241,-1019 Q1241,-919 1200,-836 Q1160,-754 1084,-696 Q1009,-639 904,-611 L850,-590 L397,-590 L395,-747 L737,-747 Q841,-747 910,-783 Q979,-820 1014,-882 Q1049,-944 1049,-1019 Q1049,-1103 1016,-1166 Q983,-1229 912,-1263 Q842,-1298 731,-1298 L442,-1298 L442,-0 L249,-0 L249,-1456 M1100,-0 L746,-660 L947,-661 L1306,-12 L1306,-0 L1100,-0 M1434,-529 L1434,-552 Q1434,-669 1468,-769 Q1502,-870 1566,-944 Q1630,-1019 1721,-1060 Q1812,-1102 1925,-1102 Q2039,-1102 2130,-1060 Q2222,-1019 2286,-944 Q2351,-870 2385,-769 Q2419,-669 2419,-552 L2419,-529 Q2419,-412 2385,-312 Q2351,-212 2286,-137 Q2222,-63 2131,-21 Q2041,20 1927,20 Q1813,20 1722,-21 Q1631,-63 1566,-137 Q1502,-212 1468,-312 Q1434,-412 1434,-529 M1619,-552 L1619,-529 Q1619,-448 1638,-376 Q1657,-305 1695,-250 Q1734,-195 1792,-163 Q1850,-132 1927,-132 Q2003,-132 2060,-163 Q2118,-195 2156,-250 Q2194,-305 2213,-376 Q2233,-448 2233,-529 L2233,-552 Q2233,-632 2213,-703 Q2194,-775 2155,-830 Q2117,-886 2059,-918 Q2002,-950 1925,-950 Q1849,-950 1791,-918 Q1734,-886 1695,-830 Q1657,-775 1638,-703 Q1619,-632 1619,-552 M2650,-1536 L2836,-1536 L2836,-210 L2820,-0 L2650,-0 L2650,-1536 M3567,-550 L3567,-529 Q3567,-411 3539,-310 Q3511,-210 3457,-136 Q3403,-62 3325,-21 Q3247,20 3146,20 Q3043,20 2965,-15 Q2888,-51 2835,-118 Q2782,-185 2750,-280 Q2719,-375 2707,-494 L2707,-586 Q2719,-706 2750,-801 Q2782,-896 2835,-963 Q2888,-1031 2965,-1066 Q3042,-1102 3144,-1102 Q3246,-1102 3325,-1062 Q3404,-1023 3457,-950 Q3511,-877 3539,-775 Q3567,-674 3567,-550 M3381,-529 L3381,-550 Q3381,-631 3366,-702 Q3351,-774 3318,-828 Q3285,-883 3231,-914 Q3177,-946 3098,-946 Q3028,-946 2976,-922 Q2925,-898 2889,-857 Q2853,-817 2830,-765 Q2808,-714 2797,-659 L2797,-418 Q2813,-348 2849,-283 Q2886,-219 2947,-178 Q3009,-137 3100,-137 Q3175,-137 3228,-167 Q3282,-198 3315,-252 Q3349,-306 3365,-377 Q3381,-448 3381,-529 M3752,-529 L3752,-552 Q3752,-669 3786,-769 Q3820,-870 3884,-944 Q3948,-1019 4039,-1060 Q4130,-1102 4243,-1102 Q4357,-1102 4448,-1060 Q4540,-1019 4604,-944 Q4669,-870 4703,-769 Q4737,-669 4737,-552 L4737,-529 Q4737,-412 4703,-312 Q4669,-212 4604,-137 Q4540,-63 4449,-21 Q4359,20 4245,20 Q4131,20 4040,-21 Q3949,-63 3884,-137 Q3820,-212 3786,-312 Q3752,-412 3752,-529 M3937,-552 L3937,-529 Q3937,-448 3956,-376 Q3975,-305 4013,-250 Q4052,-195 4110,-163 Q4168,-132 4245,-132 Q4321,-132 4378,-163 Q4436,-195 4474,-250 Q4512,-305 4531,-376 Q4551,-448 4551,-529 L4551,-552 Q4551,-632 4531,-703 Q4512,-775 4473,-830 Q4435,-886 4377,-918 Q4320,-950 4243,-950 Q4167,-950 4109,-918 Q4052,-886 4013,-830 Q3975,-775 3956,-703 Q3937,-632 3937,-552 M5422,-1082 L5422,-940 L4837,-940 L4837,-1082 L5422,-1082 M5035,-1345 L5220,-1345 L5220,-268 Q5220,-213 5237,-185 Q5254,-157 5281,-148 Q5308,-139 5339,-139 Q5362,-139 5387,-143 Q5413,-148 5426,-151 L5427,-0 Q5405,7 5369,14 Q5334,20 5284,20 Q5216,20 5159,-7 Q5102,-34 5068,-97 Q5035,-161 5035,-269 L5035,-1345 M5590,-529 L5590,-552 Q5590,-669 5624,-769 Q5658,-870 5722,-944 Q5786,-1019 5877,-1060 Q5968,-1102 6081,-1102 Q6195,-1102 6286,-1060 Q6378,-1019 6442,-944 Q6507,-870 6541,-769 Q6575,-669 6575,-552 L6575,-529 Q6575,-412 6541,-312 Q6507,-212 6442,-137 Q6378,-63 6287,-21 Q6197,20 6083,20 Q5969,20 5878,-21 Q5787,-63 5722,-137 Q5658,-212 5624,-312 Q5590,-412 5590,-529 M5775,-552 L5775,-529 Q5775,-448 5794,-376 Q5813,-305 5851,-250 Q5890,-195 5948,-163 Q6006,-132 6083,-132 Q6159,-132 6216,-163 Q6274,-195 6312,-250 Q6350,-305 6369,-376 Q6389,-448 6389,-529 L6389,-552 Q6389,-632 6369,-703 Q6350,-775 6311,-830 Q6273,-886 6215,-918 Q6158,-950 6081,-950 Q6005,-950 5947,-918 Q5890,-886 5851,-830 Q5813,-775 5794,-703 Q5775,-632 5775,-552'/></g></svg>
```

## Test 1: Roboto
<img src="https://user-images.githubusercontent.com/5407363/152979344-4e9da20e-fc8a-443a-ab87-ee7835c929a6.svg" width="1000">
File Size (Gzipped): 1.62KB
File Gzipped: [Roboto.svg.gz](https://github.com/clickfunnels2/admin/files/8023433/Roboto.svg.gz)

## Test 2: Architects Daughter
<img src="https://user-images.githubusercontent.com/5407363/152984941-304f89db-5100-4402-8686-ef66c613efda.svg" width="1000">
File Size (Gzipped): 4.14KB
File Gzipped: [ArchitectsDaughter.svg.gz](https://github.com/clickfunnels2/admin/files/8023708/ArchitectsDaughter.svg.gz)

## Test 3: Bebas Neue
<img src="https://user-images.githubusercontent.com/5407363/152987773-149a0e94-a69f-457e-8410-33208512d64b.svg" width="1000">
File Size (Gzipped): 1KB
File Gzipped: [BebasNeue.svg.gz](https://github.com/clickfunnels2/admin/files/8023791/BebasNeue.svg.gz)

# 3. **Use `webfontloader` with the parameter `text`**
```
WebFontConfig = {
  google: {
    families: ['Roboto'], 
    text: 'Robt' // include only these 4 glyhps in the font-face
  }
};
```
- **Disadvantage**: Two requests per font. One to retrieve the `font-face` another one to download the font with the selected glyphs
- We could build a css file with all font-faces to mitigate the problem with two requests per font, but this css would have to be updated from time to time to get google fonts API changes.

## Test 1: Roboto
font-face url: https://fonts.googleapis.com/css2?family=Roboto&text=Rbot 
font url (Only used Glyphs): https://fonts.gstatic.com/l/font?kit=KFOmCnqEu92Fr1Me4H5LUVVSXg&skey=a0a0114a1dcab3ac&v=v29
Size: 225B + 1.69KB

## Test 2: Architects Daughter
font-face url: https://fonts.googleapis.com/css2?family=Architects%20Daughter&text=%20ADaceghirstu
font url (Only used Glyphs): https://fonts.gstatic.com/l/font?kit=KtkxAKiDZI_td1Lkx62xHZHDtgO_Y-bvfYty6-SgT4To0oHhiuGvWpw&skey=d34ee9a1a308e98b&v=v17
Size: 260B + 1.75KB

## Test 3: Bebas Neue
font-face url: https://fonts.googleapis.com/css2?family=Bebas%20Neue&text=%20BNabesu
font url (Only used Glyphs): https://fonts.gstatic.com/l/font?kit=JTUSjIg69CK48gW7PXooxWtzxi6htbcXwVjW&skey=6bd981f07b300212&v=v8
Size: 233B+ 3.17KB

# TLDR
## Summary

| Font \ Strategy          | PNG (Compressed) | SVG (Gzipped) | WebFontLoader                 |
|------------------------|----------------------|-----------------|------------------------------|
| Roboto                      |   2.04KB                  |  1.62KB            |  225B + 1.69KB (1.915KB)   |
| Architects Daughter  |   4.73KB                  |  4.14KB            |  260B + 1.75KB (2KB)         |
| Bebas Neue               |   1.76KB                  |  1KB                |   233B + 3.17KB (3.4KB)      |

- The PNG option seems to be a poor choice given it performed worse in all tests.
- SVG (Gzipped) seems to be the vest option when there is not much Glyph repetitions in the font name and there is a lot of straight lines in the glyphs.
- WebFontLonder has a good avarage size for all cases and is the easiest option to use given we don't need to have a pre render process to generate the fonts. Also, they are always up to date.

## Conclusion (TLDR)

**Given that, I think the best option to follow is to use `webfontloader` with parameter text and lazy load the fonts as we scroll the list. It should give us a performance close to what we can experience in https://fonts.google.com/**

# Page Content Fonts

On page start we should load all fonts used by it into the page content iframe.

