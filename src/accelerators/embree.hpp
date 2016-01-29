#pragma once

// PBRT includes
#include <core/pbrt.h>
#include <core/primitive.h>
#include <shapes/triangle.h>

// Embree includes
#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

class EmbreeAcc : public Aggregate {
   public:
      /* Load the list of primitives and create the associated Embree scene
       * repesentation. That is we iterate over the list of primitives and
       * check if they can be converted to triangle meshes. If so, we add the
       * triangle soup to Embree using 'AddTriangleMesh'.
       * */
      EmbreeAcc(const std::vector<std::shared_ptr<Primitive>>& p) {
         device = rtcNewDevice(NULL);
         scene  = rtcDeviceNewScene(device, RTC_SCENE_STATIC, RTC_INTERSECT1);

         std::cout << "Using Embree as the main accelarator" << std::endl;
         shapes.assign(p.size(), nullptr);
         primitives = p;

         for(unsigned int i=0; i<p.size(); ++i) {
            auto prim = p[i];
            auto gp   = std::dynamic_pointer_cast<GeometricPrimitive>(prim);
            auto mesh = std::dynamic_pointer_cast<Triangle>(gp->GetShape());
            shapes[i] = mesh;

            auto gID  = rtcNewTriangleMesh(scene, RTC_GEOMETRY_STATIC, 1, 3, 1);

            // Add mesh and update bounds
            if(mesh) {
               AddTriangleMesh(mesh, gID);
               bounds = Union(bounds, mesh->WorldBound());
            }
         }

         rtcCommit(scene);
      }

      virtual ~EmbreeAcc() {
         rtcDeleteScene(scene);
         rtcDeleteDevice(device);
      }

      /* Primitive interface, returning the Aggregate's volume
       */
      Bounds3f WorldBound() const {
         return bounds;
      }

      /* TODO: Compute the correct pError
       */
      virtual bool Intersect(const Ray &r, SurfaceInteraction *isect) const {
         RTCRay ray;
         CreateRtcRay(r, ray);
         rtcIntersect(scene, ray);

         if(ray.geomID != RTC_INVALID_GEOMETRY_ID) {
            Point2f  uvHit(ray.u, ray.v);
            Point3f  pHit = r.o + ray.tfar * r.d;
            Vector3f pError(0, 0, 0);
            Vector3f dpdu, dpdv;
            shapes[ray.geomID]->SurfacePartials(dpdu, dpdv);
            *isect = SurfaceInteraction(pHit, pError, uvHit, -r.d, dpdu, dpdv,
                                        Normal3f(0, 0, 0), Normal3f(0, 0, 0),
                                        ray.time, shapes[ray.geomID].get());
            isect->primitive = primitives[ray.geomID].get();
            return true;
         } else {
            return false;
         }
      }

      /*
       */
      virtual bool IntersectP(const Ray &r) const {
         RTCRay ray;
         CreateRtcRay(r, ray);
         rtcIntersect(scene, ray);
         return ray.geomID != RTC_INVALID_GEOMETRY_ID;
      }

   private:
      void AddTriangleMesh(const std::shared_ptr<Triangle>& tri,
                           unsigned int id) {
         std::cout << "Adding a new triangle mesh" << std::endl;
         auto   trimesh  = tri->GetMesh();

         float* vertices = (float*) rtcMapBuffer(scene, id, RTC_VERTEX_BUFFER);
         vertices[ 0] = trimesh->p[tri->GetIndices()[0]].x;
         vertices[ 1] = trimesh->p[tri->GetIndices()[0]].y;
         vertices[ 2] = trimesh->p[tri->GetIndices()[0]].z;
         vertices[ 4] = trimesh->p[tri->GetIndices()[1]].x;
         vertices[ 5] = trimesh->p[tri->GetIndices()[1]].y;
         vertices[ 6] = trimesh->p[tri->GetIndices()[1]].z;
         vertices[ 8] = trimesh->p[tri->GetIndices()[2]].x;
         vertices[ 9] = trimesh->p[tri->GetIndices()[2]].y;
         vertices[10] = trimesh->p[tri->GetIndices()[2]].z;
         rtcUnmapBuffer(scene, id, RTC_VERTEX_BUFFER);

         int* triangles = (int*) rtcMapBuffer(scene, id, RTC_INDEX_BUFFER);
         triangles[0] = 0;
         triangles[1] = 1;
         triangles[2] = 2;
         rtcUnmapBuffer(scene, id, RTC_INDEX_BUFFER);
      }

      void CreateRtcRay(const Ray& r, RTCRay& ray) const {
         ray.org[0] = r.o.x;
         ray.org[1] = r.o.y;
         ray.org[2] = r.o.z;
         ray.dir[0] = r.d.x;
         ray.dir[1] = r.d.y;
         ray.dir[2] = r.d.z;
         ray.tnear  = 0.0f;
         ray.tfar   = r.tMax;
         ray.time   = r.time;
         ray.geomID = RTC_INVALID_GEOMETRY_ID;
         ray.primID = RTC_INVALID_GEOMETRY_ID;
         ray.mask   = -1;
      }

      /* Embree related structures */
      RTCScene scene;
      RTCDevice device;

      /* PBRT related structures */
      Bounds3f bounds;

      /* List of shapes */
      std::vector<std::shared_ptr<Triangle>>  shapes;
      std::vector<std::shared_ptr<Primitive>> primitives;
};

/* API interface for this Accelerator
 */
std::shared_ptr<EmbreeAcc> CreateEmbreeAccelerator(
               const std::vector<std::shared_ptr<Primitive>> &prims,
               const ParamSet &ps) {
   return std::make_shared<EmbreeAcc>(prims);
}
