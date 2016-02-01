#pragma once

// PBRT includes
#include <core/pbrt.h>
#include <core/material.h>

// ALTA includes
#include <core/params.h>
#include <core/function.h>
#include <core/data.h>
#include <core/plugins_manager.h>


struct AltaBRDF : public BxDF {
    AltaBRDF(const alta::ptr<alta::data>& d,
             const alta::ptr<alta::function>& f)
       : BxDF(BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)), _d(d), _f(f) { }

    Spectrum eval(const Vector3f &wo, const Vector3f &wi) const {
       if (wi.z <= 0 || wo.z <= 0) {
          return Spectrum(0.0f);
       }

       double cart[6];
       cart[0] = wi[0];
       cart[1] = wi[1];
       cart[2] = wi[2];
       cart[3] = wo[0];
       cart[4] = wo[1];
       cart[5] = wo[2];


       /* Return the value of the BRDF from the function object */
       if(!_d) {
          vec x(_f->dimX());
          alta::params::convert(&cart[0], alta::params::CARTESIAN, _f->input_parametrization(), &x[0]);
          vec y = _f->value(x);
          Spectrum res;
          if(_f->dimY() == 3) {
             Float rgb[3] = {Float(y[0]), Float(y[1]), Float(y[2])};
             res = RGBSpectrum::FromRGB(rgb);
          } else {
             res = RGBSpectrum(std::max(y[0], 0.0));
          }
          return res;

          /* Treat the case of a BRDF from interpolated data */
       } else {
          vec x(_d->dimX());
          alta::params::convert(&cart[0], alta::params::CARTESIAN, _d->input_parametrization(), &x[0]);

          vec y = _d->value(x);
          Spectrum res;
          if(_d->dimY() == 3) {
             Float rgb[3] = {Float(y[0]), Float(y[1]), Float(y[2])};
             res = RGBSpectrum::FromRGB(rgb);
          } else {
             res = RGBSpectrum(std::max(y[0], 0.0));
          }
          return res;
       }
    }

    Spectrum f(const Vector3f &wo, const Vector3f &wi) const {
#ifdef FORCE_BILATERAL_SYMMETRY
       return 0.5*(eval(wo, wi) + eval(wi, wo));
#else
       return eval(wo, wi);
#endif
    }

    const alta::ptr<alta::data>     _d;
    const alta::ptr<alta::function> _f;
};

class AltaMaterial : public Material {
   public:
      AltaMaterial(const std::string& filename,
                   const std::string& plugin,
                   const std::string& plugin_type) {

         // Load the ALTA material
         if(plugin_type == "data") {
            _data = alta::ptr<alta::data>(alta::plugins_manager::get_data(plugin));
            _data->load(filename);
         } else {
            _func = alta::ptr<alta::function>(alta::plugins_manager::load_function(filename));
         }
      }

      virtual void ComputeScatteringFunctions(SurfaceInteraction* si,
                                              MemoryArena& arena,
                                              TransportMode mode,
                                              bool allowMultipleLobes) const {

         si->bsdf = ARENA_ALLOC(arena, BSDF)(*si);
         si->bsdf->Add(ARENA_ALLOC(arena, AltaBRDF)(_data, _func));
      }
   private:
      // ALTA internal types
      alta::ptr<alta::function> _func;
      alta::ptr<alta::data>     _data;
};

AltaMaterial* CreateAltaMaterial(const TextureParams& mp) {
   return new AltaMaterial(mp.FindFilename("filename"),
                           mp.FindString("plugin"),
                           mp.FindString("plugin-type"));
}

