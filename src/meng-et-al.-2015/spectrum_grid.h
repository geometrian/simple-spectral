#ifndef SPECTRUM_GRID_H
#define SPECTRUM_GRID_H

#include "spectra_xyz_5nm_380_780_0.97.h"

#include <math.h>
#include <float.h>
#include <assert.h>

/*
 * Evaluate the spectrum for xyz at the given wavelength.
 */
static inline float spectrum_xyz_to_p(const float lambda, const float *xyz)
{
  assert(lambda >= spectrum_sample_min);
  assert(lambda <= spectrum_sample_max);
  float xyY[3], uv[2];
  
  const float norm = 1.0/(xyz[0] + xyz[1] + xyz[2]);
  if(!(norm < FLT_MAX)) 
  {
      return 0.0f;
  }
  // convert to xy chromaticities
  xyY[0] = xyz[0] * norm;
  xyY[1] = xyz[1] * norm;
  xyY[2] = xyz[1];

  // rotate to align with grid
  spectrum_xy_to_uv(xyY, uv);

  if (uv[0] < 0.0f || uv[0] >= spectrum_grid_width || 
      uv[1] < 0.0f || uv[1] >= spectrum_grid_height)
  {
      return 0.f;
  }

  int uvi[2] = {(int)uv[0], (int)uv[1]};
  assert(uvi[0] < spectrum_grid_width);
  assert(uvi[1] < spectrum_grid_height);

  const int cell_idx = uvi[0] + spectrum_grid_width * uvi[1];
  assert(cell_idx < spectrum_grid_width*spectrum_grid_height);
  assert(cell_idx >= 0);
	
  const spectrum_grid_cell_t* cell = spectrum_grid + cell_idx;
  const int inside = cell->inside;
  const int *idx   = cell->idx;
  const int num    = cell->num_points;

  // get linearly interpolated spectral power for the corner vertices:
  //float p[num];
  float* p = (float*)alloca(num*sizeof(float));
  // this clamping is only necessary if lambda is not sure to be >= spectrum_sample_min and <= spectrum_sample_max:
  const float sb = //fminf(spectrum_num_samples-1e-4, fmaxf(0.0f,
        (lambda - spectrum_sample_min)/(spectrum_sample_max-spectrum_sample_min) * (spectrum_num_samples-1);//));
  assert(sb >= 0.f);
  assert(sb <= spectrum_num_samples);

  const int sb0 = (int)sb;
  const int sb1 = sb+1 < spectrum_num_samples ? sb+1 : spectrum_num_samples-1;
  const float sbf = sb - sb0;
  for(int i=0; i<num; ++i)
  {
	  assert(idx[i] >= 0);
	  assert(sb0 < spectrum_num_samples);
	  assert(sb1 < spectrum_num_samples);
	  const float* spectrum = spectrum_data_points[idx[i]].spectrum;
      p[i] = spectrum[sb0] * (1.0f-sbf) + spectrum[sb1] * sbf;
  }

  float interpolated_p = 0.0f;

  if(inside)
  { // fast path for normal inner quads:
    uv[0] -= uvi[0];
    uv[1] -= uvi[1];

    assert(uv[0] >= 0 && uv[0] <= 1.f);
    assert(uv[1] >= 0 && uv[1] <= 1.f);

    // the layout of the vertices in the quad is:
    //  2  3
    //  0  1
    interpolated_p = 
      p[0] * (1.0f-uv[0]) * (1.0f-uv[1]) + p[2] * (1.0f-uv[0])  * uv[1] +
      p[3] * uv[0]        * uv[1]        + p[1] * uv[0]         * (1.0f-uv[1]);
  }
  else
  { // need to go through triangulation :(
    // we get the indices in such an order that they form a triangle fan around idx[0].
    // compute barycentric coordinates of our xy* point for all triangles in the fan:
    const float ex = uv[0] - spectrum_data_points[idx[0]].uv[0];
    const float ey = uv[1] - spectrum_data_points[idx[0]].uv[1];
    float e0x = spectrum_data_points[idx[1]].uv[0] - spectrum_data_points[idx[0]].uv[0];
    float e0y = spectrum_data_points[idx[1]].uv[1] - spectrum_data_points[idx[0]].uv[1];
    float uu = e0x*ey - ex*e0y;
    for(int i=0;i<num-1;i++)
    {
      float e1x, e1y;
      if(i == num-2)
      { // close the circle
        e1x = spectrum_data_points[idx[1]].uv[0] - spectrum_data_points[idx[0]].uv[0];
        e1y = spectrum_data_points[idx[1]].uv[1] - spectrum_data_points[idx[0]].uv[1];
      }
      else
      {
        e1x = spectrum_data_points[idx[i+2]].uv[0] - spectrum_data_points[idx[0]].uv[0];
        e1y = spectrum_data_points[idx[i+2]].uv[1] - spectrum_data_points[idx[0]].uv[1];
      }
      float vv = ex*e1y - e1x*ey;

      // TODO: with some sign magic, this division could be deferred to the last iteration!
      const float area = e0x*e1y - e1x*e0y;
      // normalise 
      const float u = uu / area;
      const float v = vv / area;
      float w = 1.0f - u - v;
      // outside spectral locus (quantized version at least) or outside grid
      if(u < 0.0 || v < 0.0 || w < 0.0)
      {
        uu = -vv;
        e0x = e1x;
        e0y = e1y;
        continue;
      }

      // This seems to be the triangle we've been looking for.
      interpolated_p = p[0] * w + p[i+1] * v + p[(i == num-2) ? 1 : (i+2)] * u;
      break;
    }
  }

  // now we have a spectrum which corresponds to the xy chromaticities of the input. need to scale according to the
  // input brightness X+Y+Z now:
  return interpolated_p / norm;
}

#endif

