# Data For "A Low-Dimensional Function Space for Efficient Spectral Upsampling"

These two files are copies of those that were helpfully provided by Jakob and Hanika in their EG 2019 paper "A Low-Dimensional Function Space for Efficient Spectral Upsampling".  The data was downloaded from the Realistic Graphics Lab's [website](https://rgl.epfl.ch/publications/Jakob2019Spectral).

We added `const` in a few places to their code and changed `RGB2Spec` so that it can be forward-declared.  Please be aware that, for simplicity, we do their conversion from RGB to the intermediary coefficients on-the-fly instead of in a preprocess.

A copy of the paper per-se can be found on the same site.
