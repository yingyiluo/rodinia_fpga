#include "timer.h"
#include "debug_defines.h"
#include "../common/debug/manager.cl"
#include "common.h"
#include "kernel_common.h"

#define shift_reg

__kernel void compute_step_factor(__global float* restrict v_density,
                                  __global float* restrict v_momentum_x,
                                  __global float* restrict v_momentum_y,
                                  __global float* restrict v_momentum_z,
                                  __global float* restrict v_energy,
                                  __global float* restrict areas, 
                                  __global float* restrict step_factors,
                                  int nelr){
  for (int i = 0; i < nelr; ++i) {

    float density = v_density[i];
    FLOAT3 momentum;
    momentum.x = v_momentum_x[i];
    momentum.y = v_momentum_y[i];
    momentum.z = v_momentum_z[i];
	
    float density_energy = v_energy[i];
	
    FLOAT3 velocity;       compute_velocity(density, momentum, &velocity);
    float speed_sqd      = compute_speed_sqd(velocity);
    //float speed_sqd;
    //compute_speed_sqd(velocity, speed_sqd);
    float pressure       = compute_pressure(density, density_energy, speed_sqd);
    float speed_of_sound = compute_speed_of_sound(density, pressure);

    // dt = float(0.5f) * sqrtf(areas[i]) /  (||v|| + c).... but when we do time stepping, this later would need to be divided by the area, so we just do it all at once
    //step_factors[i] = (float)(0.5f) / (sqrtf(areas[i]) * (sqrtf(speed_sqd) + speed_of_sound));
    step_factors[i] = (float)(0.5f) / (sqrt(areas[i]) * (sqrt(speed_sqd) + speed_of_sound));
  }
}

__kernel void compute_flux(
    __global int* restrict elements_surrounding_elements, 
    __global float* restrict normals, 
    __global float* restrict v_density,
    __global float* restrict v_momentum_x,
    __global float* restrict v_momentum_y,
    __global float* restrict v_momentum_z,
    __global float* restrict v_energy,
    __constant float* restrict ff_variable,
    __global float* restrict fluxes_density,
    __global float* restrict fluxes_momentum_x,
    __global float* restrict fluxes_momentum_y,
    __global float* restrict fluxes_momentum_z,
    __global float* restrict fluxes_energy,
    __constant FLOAT3* restrict ff_flux_contribution_density_energy,
    __constant FLOAT3* restrict ff_flux_contribution_momentum_x,
    __constant FLOAT3* restrict ff_flux_contribution_momentum_y,
    __constant FLOAT3* restrict ff_flux_contribution_momentum_z,
    int nelr){
  const float smoothing_coefficient = 0.2f;
  __local stamp_t buf[SIZE_II];

  for (int i = 0; i < nelr; ++i) {

    monitor_ii_1(buf, i);
    int j, nb;
    FLOAT3 normal; float normal_len;
    float factor;
	
    float density_i = v_density[i];
    FLOAT3 momentum_i;
    momentum_i.x = v_momentum_x[i];
    momentum_i.y = v_momentum_y[i];
    momentum_i.z = v_momentum_z[i];

    float density_energy_i = v_energy[i];

    FLOAT3 velocity_i;           compute_velocity(density_i, momentum_i, &velocity_i);
    float speed_sqd_i = compute_speed_sqd(velocity_i);
    //float speed_sqd_i;
    //compute_speed_sqd(velocity_i, speed_sqd_i);
    //float speed_i                              = sqrtf(speed_sqd_i);
    float speed_i = sqrt(speed_sqd_i);
    float pressure_i = compute_pressure(density_i, density_energy_i, speed_sqd_i);
    float speed_of_sound_i                     = compute_speed_of_sound(density_i, pressure_i);
    FLOAT3 flux_contribution_i_momentum_x, flux_contribution_i_momentum_y, flux_contribution_i_momentum_z;
    FLOAT3 flux_contribution_i_density_energy;	
    compute_flux_contribution(density_i, momentum_i, density_energy_i, pressure_i, velocity_i, &flux_contribution_i_momentum_x, &flux_contribution_i_momentum_y, &flux_contribution_i_momentum_z, &flux_contribution_i_density_energy);

    float flux_i_density_nb[NNB];
    FLOAT3 flux_i_momentum_nb[NNB];
    float flux_i_density_energy_nb[NNB];

    // Logic not sufficient for de5net_a7
    //#pragma unroll
    for(j = 0; j < NNB; j++)
    {
      nb = elements_surrounding_elements[i + j*nelr];
      normal.x = normals[i + (j + 0*NNB)*nelr];
      normal.y = normals[i + (j + 1*NNB)*nelr];
      normal.z = normals[i + (j + 2*NNB)*nelr];
      //normal_len = sqrtf(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
      normal_len = sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);

      float fim_x = 0.0f, fim_y = 0.0f, fim_z = 0.0f;
      float fi_density = 0.0f, fi_energy = 0.0f;
		
      if(nb >= 0) 	// a legitimate neighbor
      {
        float density_nb = v_density[nb];
        FLOAT3 momentum_nb;
        momentum_nb.x = v_momentum_x[nb];
        momentum_nb.y = v_momentum_y[nb];
        momentum_nb.z = v_momentum_z[nb];
        float density_energy_nb = v_energy[nb];
        FLOAT3 velocity_nb;
        compute_velocity(density_nb, momentum_nb, &velocity_nb);
        float speed_sqd_nb                      = compute_speed_sqd(velocity_nb);
        float pressure_nb                       = compute_pressure(density_nb, density_energy_nb, speed_sqd_nb);
        float speed_of_sound_nb                 = compute_speed_of_sound(density_nb, pressure_nb);
        FLOAT3 flux_contribution_nb_density_energy;
        FLOAT3 flux_contribution_nb_momentum_x,
            flux_contribution_nb_momentum_y,
            flux_contribution_nb_momentum_z;        
        compute_flux_contribution(density_nb, momentum_nb, density_energy_nb,
                                  pressure_nb, velocity_nb,
                                  &flux_contribution_nb_momentum_x,
                                  &flux_contribution_nb_momentum_y,
                                  &flux_contribution_nb_momentum_z,
                                  &flux_contribution_nb_density_energy);
			
        // artificial viscosity
        factor = -normal_len*smoothing_coefficient*(float)(0.5f)*(speed_i + sqrt(speed_sqd_nb) + speed_of_sound_i + speed_of_sound_nb);
        fi_density += factor*(density_i-density_nb);
        fi_energy += factor*(density_energy_i-density_energy_nb);
        fim_x += factor*(momentum_i.x-momentum_nb.x);
        fim_y += factor*(momentum_i.y-momentum_nb.y);        
        fim_z += factor*(momentum_i.z-momentum_nb.z);

        // accumulate cell-centered fluxes
        factor = (float)(0.5f)*normal.x;
        fi_density += factor*(momentum_nb.x+momentum_i.x);
        fi_energy += factor*(flux_contribution_nb_density_energy.x+flux_contribution_i_density_energy.x);
        fim_x += factor*(flux_contribution_nb_momentum_x.x+flux_contribution_i_momentum_x.x);        
        fim_y += factor*(flux_contribution_nb_momentum_y.x+flux_contribution_i_momentum_y.x);
        fim_z += factor*(flux_contribution_nb_momentum_z.x+flux_contribution_i_momentum_z.x);
        
        factor = (float)(0.5f)*normal.y;
        fi_density += factor*(momentum_nb.y+momentum_i.y);
        fi_energy += factor*(flux_contribution_nb_density_energy.y+flux_contribution_i_density_energy.y);
        fim_x += factor*(flux_contribution_nb_momentum_x.y+flux_contribution_i_momentum_x.y);        
        fim_y += factor*(flux_contribution_nb_momentum_y.y+flux_contribution_i_momentum_y.y);
        fim_z += factor*(flux_contribution_nb_momentum_z.y+flux_contribution_i_momentum_z.y);
        
        factor = (float)(0.5f)*normal.z;
        fi_density += factor*(momentum_nb.z+momentum_i.z);
        fi_energy += factor*(flux_contribution_nb_density_energy.z+flux_contribution_i_density_energy.z);
        fim_x += factor*(flux_contribution_nb_momentum_x.z+flux_contribution_i_momentum_x.z);        
        fim_y += factor*(flux_contribution_nb_momentum_y.z+flux_contribution_i_momentum_y.z);
        fim_z += factor*(flux_contribution_nb_momentum_z.z+flux_contribution_i_momentum_z.z);
      }
      else if(nb == -1)	// a wing boundary
      {
        //flux_i_momentum.x += normal.x*pressure_i;
        fim_x += normal.x*pressure_i;        
        //flux_i_momentum.y += normal.y*pressure_i;
        fim_y += normal.y*pressure_i;        
        //flux_i_momentum.z += normal.z*pressure_i;
        fim_z += normal.z*pressure_i;
      }
      else if(nb == -2) // a far field boundary
      {
        factor = (float)(0.5f)*normal.x;
        fi_density += factor*(ff_variable[VAR_MOMENTUM+0]+momentum_i.x);
        fi_energy += factor*(ff_flux_contribution_density_energy[0].x+flux_contribution_i_density_energy.x);
        fim_x += factor*(ff_flux_contribution_momentum_x[0].x + flux_contribution_i_momentum_x.x);
        fim_y += factor*(ff_flux_contribution_momentum_y[0].x + flux_contribution_i_momentum_y.x);
        fim_z += factor*(ff_flux_contribution_momentum_z[0].x + flux_contribution_i_momentum_z.x);
			
        factor = (float)(0.5f)*normal.y;
        fi_density += factor*(ff_variable[VAR_MOMENTUM+1]+momentum_i.y);
        fi_energy += factor*(ff_flux_contribution_density_energy[0].y+flux_contribution_i_density_energy.y);
        fim_x += factor*(ff_flux_contribution_momentum_x[0].y + flux_contribution_i_momentum_x.y);
        fim_y += factor*(ff_flux_contribution_momentum_y[0].y + flux_contribution_i_momentum_y.y);
        fim_z += factor*(ff_flux_contribution_momentum_z[0].y + flux_contribution_i_momentum_z.y);

        factor = (float)(0.5f)*normal.z;
        fi_density += factor*(ff_variable[VAR_MOMENTUM+2]+momentum_i.z);
        fi_energy += factor*(ff_flux_contribution_density_energy[0].z+flux_contribution_i_density_energy.z);
        fim_x += factor*(ff_flux_contribution_momentum_x[0].z + flux_contribution_i_momentum_x.z);
        fim_y += factor*(ff_flux_contribution_momentum_y[0].z + flux_contribution_i_momentum_y.z);
        fim_z += factor*(ff_flux_contribution_momentum_z[0].z + flux_contribution_i_momentum_z.z);
      }

      flux_i_momentum_nb[j].x = fim_x;
      flux_i_momentum_nb[j].y = fim_y;
      flux_i_momentum_nb[j].z = fim_z;
      flux_i_density_nb[j] = fi_density;
      flux_i_density_energy_nb[j] = fi_energy;
    }

    float flux_i_density = 0.0f;
    FLOAT3 flux_i_momentum;
    flux_i_momentum.x = 0.0f;
    flux_i_momentum.y = 0.0f;
    flux_i_momentum.z = 0.0f;        
    float flux_i_density_energy = 0.0f;
#pragma unroll
    for(j = 0; j < NNB; j++) {
      flux_i_density += flux_i_density_nb[j];
      flux_i_momentum.x += flux_i_momentum_nb[j].x;
      flux_i_momentum.y += flux_i_momentum_nb[j].y;
      flux_i_momentum.z += flux_i_momentum_nb[j].z;
      flux_i_density_energy += flux_i_density_energy_nb[j];
    }
    fluxes_density[i] = flux_i_density;
    fluxes_momentum_x[i] = flux_i_momentum.x;
    fluxes_momentum_y[i] = flux_i_momentum.y;
    fluxes_momentum_z[i] = flux_i_momentum.z;
    fluxes_energy[i] = flux_i_density_energy;
  }
  finish_monitor_ii(buf, 0);
}

__kernel void time_step(int j, int nelr, 
                        __global float* restrict old_v_density,
                        __global float* restrict old_v_momentum_x,
                        __global float* restrict old_v_momentum_y,
                        __global float* restrict old_v_momentum_z,
                        __global float* restrict old_v_energy,                        
                        __global float* restrict v_density,
                        __global float* restrict v_momentum_x,
                        __global float* restrict v_momentum_y,
                        __global float* restrict v_momentum_z,
                        __global float* restrict v_energy,
                        __global float* restrict step_factors, 
                        __global float* restrict fluxes_density,
                        __global float* restrict fluxes_momentum_x,
                        __global float* restrict fluxes_momentum_y,
                        __global float* restrict fluxes_momentum_z,
                        __global float* restrict fluxes_energy) {
  for (int i = 0; i < nelr; ++i) {
    float factor = step_factors[i]/(float)(RK+1-j);

    v_density[i] = old_v_density[i] + factor*fluxes_density[i];
    v_momentum_x[i] = old_v_momentum_x[i] + factor*fluxes_momentum_x[i];
    v_momentum_y[i] = old_v_momentum_y[i] + factor*fluxes_momentum_y[i];
    v_momentum_z[i] = old_v_momentum_z[i] + factor*fluxes_momentum_z[i];
    v_energy[i] = old_v_energy[i] + factor*fluxes_energy[i];
  }
}


