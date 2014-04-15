GLTC
====

Comprehensive Texture Compression Utility for OpenGL


Supported Texture Compression Formats
=====================================

* ETC
* DXTC (S3TC)
* PVRTC



ETC/ETC2
========

For indepth details please refer to the OpenGL|ES 3 specifications.

Image is composed of individual 4x4 blocks. Each block can be encoded with 2/5 modes. While ETC only supports RGB images
ETC2 supports RGB, RGB with punch-thru alpha and full RGBA. The modes are:

  * Individual-Mode
  * Differential-Mode
  * T-Mode      (ETC2-only)
  * H-Mode      (ETC2-only)
  * Planar-Mode (ETC2-only)

ETC is historically derived from another format called PACKMAN which encoded images in 2x4 blocks. ETC expands it to use
4x4 blocks. So in the first Individual-Mode it is technically identical to PACKMAN while it is not binary compatible.
Those are two PACKMAN blocks next to each other or on top of each other. This _Flipping_ is a new degree of freedom
introduced by ETC. The pixels are encoded in column major order as illustrated here:

```
 no flip         flip
+-----+-----+   +---------+
| a e | i m |   | a e i m |
| b f | j n |   | b f j n |
| c g | k o |   +---------+
| d h | l p |   | c g k o |
+-----+-----+   | d h l p |
                +---------+
```

All modes except Planar-Mode actually operate in the HSL color space. This is because in this modes the block or
sub-block palette is computed by using one base color that is lightened or darkened by adding or subtracting a value
from a table to all color components atonce. Saturation of one component has to be considered. This is just like in HSL
color space which has the form of two cones connected at the base.

```
L (from bottom to top)
^    /\
|   /  \
|  /    \
| +------+ H (goes around the cone)
|  \    /
|   \  /
|    \/
      ----> S (from the center to the border)
```

Adding or subtracting a value from all color components atonce is equivalent to moving along L in the HSL color space.
As one can see from the shape moving along L can cause clipping in S such that in some modes there is no way to
represent a color with maximum color saturation. So in those modes it is not possible to have a fully saturated red
(#FF0000). This is becaues except in T-Mode there is always a value different from 0 that is added or subtracted from
the base color.

There are also some noteworth differences in the 5 modes:

  * Individual- and Differential-Mode have due to the sub-block design a total palette of 8 colors per block divided
    in 4 colors per sub-block. There is so to say 4 lightness settings for one chroma (combination of H and S) per
    sub-block. Such that these Modes operate best on blocks with very low variance in chroma but high variance in L as
    there are 4 lightness settings per sub-block.
  * T- and H-Mode have a total of 4 colors for the entire block. But there are 2 chromas and either 1 lightness setting
    for one and 2 lightness settings for the other chroma or 2 lightness settings for both chromas. So these modes are
    suited best for Blocks with high variance in chroma but low variance in lightness.
    NOTE:
    If the chromas can be (almost completely) separated by dividing the block horizontally or vertically, then the
    previously mentioned modes might be better suited.
  * Planar-Mode does not have a palette but instead consists of 3 colors that are being blend into each other. So in
    this mode you cannot pick a color from a palette for each pixel. This mode aims to encode blocks with very smooth
    color transitions or gradients.



Individual
----------
In this mode for each sub-block consists of a _Color_ in RGB4 (4bits per color channel) and a _Table-Index_ to construct
a palette of 4 colors and a set of 2bit palette indices for each pixel.

For an exhaustive search for the optimum encoding of a 4x4 block of the input image we need to consider:

  * 2  flips
  * 2  sub-blocks
  * 16 values per channel (and 3 channels)
  * 8  table indices

The sub total is 2 * 16 * 16 * 16 * 8 = 131,072 possible combinations to pick the optimum.

Sub-blocks with a uniform color can be constructed much easier by using a precomputed LUT (Look Up Table).
One has just to pick the _Table-Index_ and _Palette-Index_ and look up the components value such the the error is a
minimum. The worst squared error per pixel for a uniform colored sub-block in this mode is 40.

A straight forward approach would be to find a _Color_ that represents the pixel values of the sub-block in a reasonable
way such as the center color, average or median. Then compute the spread along the diagonal in the RGB space and pick
the _Table-Index_ that matches the span.



Differential
------------
In this mode the first sub-block consists of a _ColorA_ in RGB5 (5bits per color channel) and a _Table-Index_ to
construct a palette of 4 colors and a set of 2bit palette indices for each pixel. The _dColor_ of the second sub-block
is stored as dRGB3 (3bits per color component) such that the actual _ColorB_ for the second sub-block is:

  _ColorB_ = _ColorA_ + _dColor_

So we gain a higher per color channel accuracy by giving up some freedom to pick the color for the sub-blocks sparately.

For an exhaustive search for the optimum encoding of a 4x4 block of the input image we need to consider:

  * 2  flips
  * 2  sub-blocks
  * 32 values per channel (and 3 channels)
  * 8  table indices

The sub total is 2 * 2 * 32 * 32 * 32 * 8 = 1,048,576 possible combinations to pick the optimum. One may ask why both
sub-blocks are considered at 5bits per color component. And the answer is thats the resulting color resolution. One may
compute the errors for the entire search space for the second sub-block and store it in a table and then look it up
while searching the best combined error for the first and second sub-block atonce.

Blocks or the first sub-block with a uniform color can be constructed much easier by using another precomputed LUT. The
worst squared error per pixel for a uniform colored block or the first sub-block in this mode is 14. This is due to the
higher color space resolution. This does not apply per se to the second sub-block as it is dependent on the first block.
Picking the best color for a uniform first sub-block may increase the error in the second sub-block such that in general
this does only result in the best possible quality when applied to uniform blocks instead of sub-blocks.

The same straight forward approach as for the individual mode can be used but with the constraint that the result has to
be rejected if the sub-block _Colors_ differ too much.



T-Mode
------
This is an extension used in ETC2 to overcome some deficites in ETC. While in the Individual- and Differential-Mode you
have one chroma and four luminances per sub-block, here you have two chromas and one or three luminances per block. So
this mode is completely different as it does not have sub-blocks. Instead of a palette of four colors per sub-block you
now have four colors for the entire block. Intended for use on Blocks that have a high variance in chroma in a way that
cannot be separated horizontally or vertically.

The T-Mode consists of the colors _ColorA_ and _ColorB_ in RGB4 (4bits per color channel) and a _Table-Index_ to
construct a palette of four colors and a set of 2bit palette indices for each pixel. The _Table-Index_ only operates on
_ColorB_. As the original values of _ColorA_ and _ColorB_ are part of the palette, this is the only mode where you can
render a color at full chrominance.

For an exhaustive search for the optimum encoding of a 4x4 block of the input image we need to consider:

  * 16 values per channel of _ColorA_ (and 3 channels)
  * 16 values per channel of _ColorB_ (and 3 channels)
  * 8  table indices

The sub total is 16 * 16 * 16 * 16 * 16 * 16 * 8 = 134,217,728 possible combinations to pick the optimum. So giving up
the constraint of one color per sub-block to two independant colors per block dramatically increases the search space.

Blocks with a uniform color can be constructed much easier by using another precomputed LUT. The worst squared error per
pixel for a uniform colored block or the first sub-block in this mode is ???. Almost like in the Individual-Mode. But as
we only use one of those two individual colors you need to consider only 3 of 4 _Table-Indices_.

A straight forward approach would be to first isolate two color clusters. Then find a _Color_ for each cluster that
represents the pixel values of the cluster in a reasonable way such as the center color, average or median. The _Color_
of the cluster with the higher spread along the diagonal in the RGB space will be _ColorB_ and one has to pick a
_Table-Index_ that matches the span.



H-Mode
------
Similar to the T-Mode this mode uses two chromas to per block but this time with two luminances for each. Again leading
to the situation that no fully saturated colors can be reconstructed.

Almost identical to the T-Mode the only difference is that both primary colors are altered by the value provided by the
_Table-Index_ and are not part of the resulting palette. So this time both colors are treated equally leading to an
ambiguity which is also known from DXTC (S3TC). The colors would be interchangeable and with the correct palette indices
the image could be reconstructed indentically. A fact that is used for a second mode in DXTC. But this is not the case
here. Here the ambiguity is used to store the LSB (least significant bit) of the _Table-Index_. So the search-space is
just half as big as it would be if the ambiguity would be allowed.

For an exhaustive search for the optimum encoding of a 4x4 block of the input image we need to consider:

  * 16 values per channel of _ColorA_ (and 3 channels)
  * 16 values per channel of _ColorB_ (and 3 channels)
  * 8  table indices
  * 1/2 to disallow the ambiguity

The sub total is 16 * 16 * 16 * 16 * 16 * 16 * 8 / 2 = 67,108,864 possible combinations to pick the optimum. So this
time the search-space is just half as large as in the T-Mode.

It is useless to treat uniform colored blocks in this mode as the palette offers just a sub-set of the colors available
in T-Mode.

The same straight forward approach can be taken as for the T-Mode but this time the span of both clusters has to be
considered while picking the _Table-Index_.




Coding Conventions
==================

w   width of the input image
h   height of the input image
x   absolute x-coordinate within the input image
y   absolute y-coordiante within the image image

bx  x-coordinate within a block
by  y-coordinate within a block

sbx x-coordinate within a sub-block
sby y-coordinate within a sub-block
