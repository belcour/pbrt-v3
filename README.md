This fork of [PBRT v3](pbrt) provides two additionnal features:

   + Using [ALTA](alta) to render acquiered or fitted BRDF. This new material can be used using the `alta` name.
   + Using [Embree](embree) as the intersection accelerator. When you feel that PBRT's internal intersection structure is too slow for your taste, use `embree` as a replacement of `bvh` or `kdtree`.


### ALTA interface

Here is an example using the `blue-metallic-paint` from the MERL database in
PBRT using ALTA material:

   MakeNamedMaterial "blue-metallic-paint" "string type" "alta" "string filename" "./materials/blue-metallic-paint.binary" "string plugin" "data_merl" "string plugin-type" "data"

A fitted material can be used similarly:

   MakeNamedMaterial "fitted-alta" "string type" "alta" "string filename" "fit-filename.func"

   [pbrt]: https://www.github.com/mmp/pbrt-v3/
   [alta]: http://http://alta.gforge.inria.fr/
   [embree]: http://embree.github.io/
