
/*
    pbrt source code Copyright(c) 1998-2010 Matt Pharr and Greg Humphreys.

    This file is part of pbrt.

    pbrt is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.  Note that the text contents of
    the book "Physically Based Rendering" are *not* licensed under the
    GNU GPL.

    pbrt is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */


// renderers/surfacepoints.cpp*
#include "stdafx.h"
#include "renderers/surfacepoints.h"
#include "paramset.h"
#include "octree.h"
#include "camera.h"
#include "probes.h"
#include "parallel.h"
#include "progressreporter.h"
#include "scene.h"
#include "intersection.h"
#include "montecarlo.h"
#include "shapes/sphere.h"

// SurfacePointsRenderer Local Declarations
class SurfacePointTask : public Task {
public:
    SurfacePointTask(const Scene *sc, const PbrtPoint &org, float ti, int tn,
        float msd, int mf, RWMutex &m, int &rf, int &mrf,
        int &tpt, int &trt, int &npa, GeometricPrimitive &sph,
        Octree<SurfacePoint> &oct, vector<SurfacePoint> &sps,
        ProgressReporter &pr)
        : taskNum(tn), scene(sc), origin(org), time(ti),
          minSampleDist(msd), maxFails(mf), mutex(m),
          repeatedFails(rf), maxRepeatedFails(mrf), totalPathsTraced(tpt),
          totalRaysTraced(trt), numPointsAdded(npa), sphere(sph),
          octree(oct), surfacePoints(sps), prog(pr) { }
    void Run();

    int taskNum;
    const Scene *scene;
    PbrtPoint origin;
    float time;
    float minSampleDist;
    int maxFails;

    RWMutex &mutex;
    int &repeatedFails, &maxRepeatedFails;
    int &totalPathsTraced, &totalRaysTraced, &numPointsAdded;
    GeometricPrimitive &sphere;
    Octree<SurfacePoint> &octree;
    vector<SurfacePoint> &surfacePoints;
    ProgressReporter &prog;
};


struct PoissonCheck {
    PoissonCheck(float md, const PbrtPoint &pt)
        { maxDist2 = md * md; failed = false; p = pt; }
    float maxDist2;
    bool failed;
    PbrtPoint p;
    bool operator()(const SurfacePoint &sp) {
        if (DistanceSquared(sp.p, p) < maxDist2) {
            failed = true; return false;
        }
        return true;
    }
};



// SurfacePointsRenderer Method Definitions
Spectrum SurfacePointsRenderer::Li(const Scene *scene,
    const RayDifferential &ray, const Sample *sample, RNG &rng, MemoryArena &arena,
    Intersection *isect, Spectrum *T) const {
    return 0.f;
}


Spectrum SurfacePointsRenderer::Transmittance(const Scene *scene, const RayDifferential &ray,
    const Sample *sample, RNG &rng, MemoryArena &arena) const {
    return 0.f;
}


void SurfacePointsRenderer::Render(const Scene *scene) {
    // Declare shared variables for Poisson point generation
    BBox octBounds = scene->WorldBound();
    octBounds.Expand(.001f * powf(octBounds.Volume(), 1.f/3.f));
    Octree<SurfacePoint> pointOctree(octBounds);

    // Create scene bounding sphere to catch rays that leave the scene
    PbrtPoint sceneCenter;
    float sceneRadius;
    scene->WorldBound().BoundingSphere(&sceneCenter, &sceneRadius);
    Transform ObjectToWorld(Translate(sceneCenter - PbrtPoint(0,0,0)));
    Transform WorldToObject(Inverse(ObjectToWorld));
    Reference<Shape> sph = new Sphere(&ObjectToWorld, &WorldToObject,
        true, sceneRadius, -sceneRadius, sceneRadius, 360.f);
    Reference<Material> nullMaterial = Reference<Material>(NULL);
    GeometricPrimitive sphere(sph, nullMaterial, NULL);
    int maxFails = 2000, repeatedFails = 0, maxRepeatedFails = 0;
    if (PbrtOptions.quickRender) maxFails = max(10, maxFails / 10);
    int totalPathsTraced = 0, totalRaysTraced = 0, numPointsAdded = 0;
    ProgressReporter prog(maxFails, "Depositing samples");
    // Launch tasks to trace rays to find Poisson points
    PBRT_SUBSURFACE_STARTED_RAYS_FOR_POINTS();
    vector<Task *> tasks;
    RWMutex *mutex = RWMutex::Create();
    int nTasks = NumSystemCores();
    for (int i = 0; i < nTasks; ++i)
        tasks.push_back(new SurfacePointTask(scene, pCamera, time, i,
            minDist, maxFails, *mutex, repeatedFails, maxRepeatedFails,
            totalPathsTraced, totalRaysTraced, numPointsAdded, sphere, pointOctree,
            points, prog));
    EnqueueTasks(tasks);
    WaitForAllTasks();
    for (uint32_t i = 0; i < tasks.size(); ++i)
        delete tasks[i];
    RWMutex::Destroy(mutex);
    prog.Done();
    PBRT_SUBSURFACE_FINISHED_RAYS_FOR_POINTS(totalRaysTraced, numPointsAdded);
    if (filename != "") {
        // Write surface points to file
        FILE *f = fopen(filename.c_str(), "w");
        if (!f) Severe("Unable to open output file \"%s\"", filename.c_str());
        fprintf(f, "# points generated by SurfacePointsRenderer\n");
        fprintf(f, "# position (x,y,z), normal (x,y,z), area, rayEpsilon\n");
        for (u_int i = 0; i < points.size(); ++i) {
            const SurfacePoint &sp = points[i];
            fprintf(f, "%g %g %g %g %g %g %g %g\n", sp.p.x, sp.p.y, sp.p.z,
                sp.n.x, sp.n.y, sp.n.z, sp.area, sp.rayEpsilon);
        }
        fclose(f);
    }
}


void SurfacePointTask::Run() {
    // Declare common variables for _SurfacePointTask::Run()_
    RNG rng(37 * taskNum);
    MemoryArena arena;
    vector<SurfacePoint> candidates;
    while (true) {
        int pathsTraced, raysTraced = 0;
        for (pathsTraced = 0; pathsTraced < 20000; ++pathsTraced) {
            // Follow ray path and attempt to deposit candidate sample points
            Vector dir = UniformSampleSphere(rng.RandomFloat(), rng.RandomFloat());
            Ray ray(origin, dir, 0.f, INFINITY, time);
            while (ray.depth < 30) {
                // Find ray intersection with scene geometry or bounding sphere
                ++raysTraced;
                Intersection isect;
                bool hitOnSphere = false;
                if (!scene->Intersect(ray, &isect)) {
                    if (!sphere.Intersect(ray, &isect))
                        break;
                    hitOnSphere = true;
                }
                DifferentialGeometry &hitGeometry = isect.dg;
                hitGeometry.nn = Faceforward(hitGeometry.nn, -ray.d);

                // Store candidate sample point at ray intersection if appropriate
                if (!hitOnSphere && ray.depth >= 3 &&
                    isect.GetBSSRDF(RayDifferential(ray), arena) != NULL) {
                    float area = M_PI * (minSampleDist / 2.f) * (minSampleDist / 2.f);
                    candidates.push_back(SurfacePoint(hitGeometry.p, hitGeometry.nn,
                                                      area, isect.rayEpsilon));
                }

                // Generate random ray from intersection point
                Vector dir = UniformSampleSphere(rng.RandomFloat(), rng.RandomFloat());
                dir = Faceforward(dir, hitGeometry.nn);
                ray = Ray(hitGeometry.p, dir, ray, isect.rayEpsilon);
            }
            arena.FreeAll();
        }
        // Make first pass through candidate points with reader lock
        vector<bool> candidateRejected;
        candidateRejected.reserve(candidates.size());
        RWMutexLock lock(mutex, READ);
        for (uint32_t i = 0; i < candidates.size(); ++i) {
            PoissonCheck check(minSampleDist, candidates[i].p);
            octree.Lookup(candidates[i].p, check);
            candidateRejected.push_back(check.failed);
        }

        // Make second pass through points with writer lock and update octree
        lock.UpgradeToWrite();
        if (repeatedFails >= maxFails)
            return;
        totalPathsTraced += pathsTraced;
        totalRaysTraced += raysTraced;
        int oldMaxRepeatedFails = maxRepeatedFails;
        for (uint32_t i = 0; i < candidates.size(); ++i) {
            if (candidateRejected[i]) {
                // Update for rejected candidate point
                ++repeatedFails;
                maxRepeatedFails = max(maxRepeatedFails, repeatedFails);
                if (repeatedFails >= maxFails)
                    return;
            }
            else {
                // Recheck candidate point and possibly add to octree
                SurfacePoint &sp = candidates[i];
                PoissonCheck check(minSampleDist, sp.p);
                octree.Lookup(sp.p, check);
                if (check.failed) {
                    // Update for rejected candidate point
                    ++repeatedFails;
                    maxRepeatedFails = max(maxRepeatedFails, repeatedFails);
                    if (repeatedFails >= maxFails)
                        return;
                }
                else {
                    ++numPointsAdded;
                    repeatedFails = 0;
                    Vector delta(minSampleDist, minSampleDist, minSampleDist);
                    octree.Add(sp, BBox(sp.p-delta, sp.p+delta));
                    PBRT_SUBSURFACE_ADDED_POINT_TO_OCTREE(&sp, minSampleDist);
                    surfacePoints.push_back(sp);
                }
            }
        }

        // Stop following paths if not finding new points
        if (repeatedFails > oldMaxRepeatedFails) {
            int delta = repeatedFails - oldMaxRepeatedFails;
            prog.Update(delta);
        }
        if (totalPathsTraced > 50000 && numPointsAdded == 0) {
            Warning("There don't seem to be any objects with BSSRDFs "
                    "in this scene.  Giving up.");
            return;
        }
        candidates.erase(candidates.begin(), candidates.end());
    }
}


void FindPoissonPointDistribution(const PbrtPoint &pCamera, float time,
        float minDist, const Scene *scene, vector<SurfacePoint> *points) {
    SurfacePointsRenderer sp(minDist, pCamera, time, "");
    sp.Render(scene);
    points->swap(sp.points);
}


SurfacePointsRenderer *CreateSurfacePointsRenderer(const ParamSet &params,
        const PbrtPoint &pCamera, float time) {
    float minDist = params.FindOneFloat("minsampledistance", .25f);
    string filename = params.FindOneFilename("filename", "");
    if (PbrtOptions.quickRender) { minDist *= 4.f; }
    return new SurfacePointsRenderer(minDist, pCamera, time, filename);
}


