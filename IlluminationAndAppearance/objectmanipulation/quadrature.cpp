/*
 OM3D allows users to manipulate objects in photographs in 3D
 by using stock 3D models.
 Copyright (C) 2014 Natasha Kholgade and Tomas Simon
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 
 For further details, please contact the authors at nkholgad@cs.cmu.edu
 */

#include "quadrature.h"
#include "definitions.h"

Vector quadrature(PbrtPoint intersectionPt, Vector n, Ray r, BVHAccel* bvhAccel,
				  Vector* diffuseLight, Vector* specularLight, bool hasSpecular,
				  float* cTheta, float* sTheta, float* cPhi, float* sPhi, float SphereRadius,
				  int ndivs, float* groundAxis, Vector &specularColor, float* t, double* K, bool* B) {
	
	int j=0, k=0;
	float ndots, ctheta, stheta, rdotv, shininess=SHINE;
	//float rtp=(M_PI*1.0/(sqrtf(ndivs)))*(M_PI*2.0/(sqrtf(ndivs)));
	
	bool aboveground=true, didLightIntersect;
	Ray s;
	
	Vector color(.0f,.0f,.0f), ld;
	Vector reflection, view;
	
	view=-Normalize(Vector(intersectionPt));
	
	specularColor.x=.0f; specularColor.y=.0f; specularColor.z=.0f;
	
	if (t) {
		for (k=0; k<ndivs; k++) {
			t[k]=.0f;
		}
	}
	
	for (j=0; j<ndivs; j++) {
		//         shoot a ray
		
		// you know what? use the light source only if the value of the light source is
		// greater than some epsilon. That will save you a whole ton of time ;)
		//if ( (diffuseLight[j].x>MIN_LIGHT_INTENSITY || diffuseLight[j].y>MIN_LIGHT_INTENSITY || diffuseLight[j].z>MIN_LIGHT_INTENSITY) ) {
        //|| (hasSpecular && (specularLight[j].x>1e-5 || specularLight[j].y>1e-5 || specularLight[j].z > 1e-5)) ) {
		if (true) {
			
			
			//stheta=sinf(Theta[j]);
			//ctheta=cosf(Theta[j]);
			//s.d.x=stheta*cosf(Phi[j]);
			//s.d.y=stheta*sinf(Phi[j]);
			//s.d.z=ctheta;
			stheta=sTheta[j]; ctheta=cTheta[j];
			
			// for distant light sources...
#ifdef DISTANT
			s.d.x=stheta*cPhi[j];
			s.d.y=stheta*sPhi[j];
			s.d.z=ctheta;
#else
			
			// for nearby light sources...
            ld.x=SphereRadius*stheta*cPhi[j]; ld.y=SphereRadius*stheta*sPhi[j]; ld.z=SphereRadius*ctheta; // alternative format where lights are closer
            s.d.x=ld.x-intersectionPt.x;
            s.d.y=ld.y-intersectionPt.y;
            s.d.z=ld.z-intersectionPt.z;
            s.d=Normalize(s.d);
#endif
			
			s.maxt=INFINITY;
			s.mint=0;
			s.o=intersectionPt+s.d*(MIN_LIGHT_DIRECTION_ALPHA);
			// first check if the dot product is greater than zero (light source is above primitive
			ndots=Dot(n,s.d);
			//if (ndots>0) {
            if (true) {
                //if (true) {
				// then check if light source is above ground
				
				/*if (didIntersect) {
				 denom=groundAxis[0]*s.d.x+groundAxis[1]*s.d.y+groundAxis[2]*s.d.z;
				 if (fabs(denom)>1e-6) {
				 lambda=-(groundAxis[3]+
				 groundAxis[0]*intersectionPt.x+
				 groundAxis[1]*intersectionPt.y+
				 groundAxis[2]*intersectionPt.z)*1.0f/denom;
				 aboveground=lambda<1e-3;
				 } else aboveground=true;
				 
				 } else aboveground=true;*/
				// only if light source is above ground and above primitive should you
				// consider using it (this check should be done for object pixels not ground pixels)
				//aboveground=ld.x*groundAxis[0]+ld.y*groundAxis[1]+ld.z*groundAxis[2]+groundAxis[3]>0;
				aboveground=true;
				if (aboveground) {
					didLightIntersect=bvhAccel->IntersectP(s);	// COMPUTE INTENSIVE
					if (!didLightIntersect) {
						//         if it does not intersect,
						//				compute the diffuse and specular contributions --- this is something you'll have to adapt
						//              add the contribution to the running sum for the light seen at the pixel
						//color+=diffuseLight[j]*(ndots*rtp*sTheta[j]);
						
						if (t) {
							for (k=0; k<ndivs; k++) {
								if (B[k*ndivs+j]) t[k]+=ndots*K[k*ndivs+j];
							}
						}
                        
                        
						color+=diffuseLight[j]*ndots;
						if (hasSpecular) {
							reflection=Normalize(2*ndots*n-s.d);
							rdotv=Dot(reflection,view);
							if (rdotv>0) { // assume ground has no specularity for speed --- not yet
								//specularColor+=specularLight[j]*pow(rdotv, shininess);
								specularColor+=specularLight[j]*(float)exp((double)shininess*log((double)rdotv));
							}
						}
					}
				}
			}
			
		}
	}
	return color;
}


Vector quadrature(PbrtPoint intersectionPt, Vector n, Ray r, BVHAccel* bvhAccel,
				  Vector* diffuseLight, float* cTheta, float* sTheta, float* cPhi, float* sPhi, float SphereRadius,
				  int ndivs, float* groundAxis, bool isgnd, float* t, double* K, bool* B) {
	
	int j=0, k=0;
	float ndots, ctheta, stheta;
	//float rtp=(M_PI*1.0/(sqrtf(ndivs)))*(M_PI*2.0/(sqrtf(ndivs)));
	
	bool aboveground=true, didLightIntersect;
	Ray s;
	
	Vector color(.0f,.0f,.0f), ld;
	Vector reflection, view;
	
	view=-Normalize(Vector(intersectionPt));
	
	
	if (t) {
		for (k=0; k<ndivs; k++) {
			t[k]=.0f;
		}
	}
	
	for (j=0; j<ndivs; j++) {
		//         shoot a ray
		
		// you know what? use the light source only if the value of the light source is
		// greater than some epsilon. That will save you a whole ton of time ;)
		if ( (diffuseLight[j].x>MIN_LIGHT_INTENSITY || diffuseLight[j].y>MIN_LIGHT_INTENSITY || diffuseLight[j].z>MIN_LIGHT_INTENSITY) ) {
        //|| (hasSpecular && (specularLight[j].x>1e-5 || specularLight[j].y>1e-5 || specularLight[j].z > 1e-5)) ) {
		//if (true) {
			
			
			//stheta=sinf(Theta[j]);
			//ctheta=cosf(Theta[j]);
			//s.d.x=stheta*cosf(Phi[j]);
			//s.d.y=stheta*sinf(Phi[j]);
			//s.d.z=ctheta;
			stheta=sTheta[j]; ctheta=cTheta[j];
			
			// for distant light sources...
#ifdef DISTANT
			s.d.x=stheta*cPhi[j];
			s.d.y=stheta*sPhi[j];
			s.d.z=ctheta;
#else
			
			// for nearby light sources...
            ld.x=SphereRadius*stheta*cPhi[j]; ld.y=SphereRadius*stheta*sPhi[j]; ld.z=SphereRadius*ctheta; // alternative format where lights are closer
            s.d.x=ld.x-intersectionPt.x;
            s.d.y=ld.y-intersectionPt.y;
            s.d.z=ld.z-intersectionPt.z;
            s.d=Normalize(s.d);
#endif
			
			s.maxt=INFINITY;
			s.mint=0;
			s.o=intersectionPt+s.d*(MIN_LIGHT_DIRECTION_ALPHA);
			// first check if the dot product is greater than zero (light source is above primitive
			ndots=Dot(n,s.d);
			//if (ndots>0) {
            if ( !isgnd || (isgnd && ndots>0)) {
            //if (true) {
				// then check if light source is above ground
				
				/*if (didIntersect) {
				 denom=groundAxis[0]*s.d.x+groundAxis[1]*s.d.y+groundAxis[2]*s.d.z;
				 if (fabs(denom)>1e-6) {
				 lambda=-(groundAxis[3]+
				 groundAxis[0]*intersectionPt.x+
				 groundAxis[1]*intersectionPt.y+
				 groundAxis[2]*intersectionPt.z)*1.0f/denom;
				 aboveground=lambda<1e-3;
				 } else aboveground=true;
				 
				 } else aboveground=true;*/
				// only if light source is above ground and above primitive should you
				// consider using it (this check should be done for object pixels not ground pixels)
				//aboveground=ld.x*groundAxis[0]+ld.y*groundAxis[1]+ld.z*groundAxis[2]+groundAxis[3]>0;
				aboveground=true;
				if (aboveground) {
					didLightIntersect=bvhAccel->IntersectP(s);	// COMPUTE INTENSIVE
					if (!didLightIntersect) {
						//         if it does not intersect,
						//				compute the diffuse and specular contributions --- this is something you'll have to adapt
						//              add the contribution to the running sum for the light seen at the pixel
						//color+=diffuseLight[j]*(ndots*rtp*sTheta[j]);
						
						if (t) {
							for (k=0; k<ndivs; k++) {
								if (B[k*ndivs+j]) t[k]+=fabsf(ndots)*K[k*ndivs+j];
							}
						}
                        
                        
						color+=diffuseLight[j]*fabsf(ndots);
						
					}
				}
			}
			
		}
	}
	return color;
}

void quadrature(PbrtPoint intersectionPt, Vector n, Ray r, BVHAccel* bvhAccel,
				Vector** diffuseLight, Vector** specularLight, bool hasSpecular,
				float* cTheta, float* sTheta, float* cPhi, float* sPhi, float SphereRadius,
				int ndivs, float* groundAxis,
				float* shines, int nshines,
				Vector* diffuseColors, Vector* specularColors,
				PbrtPoint groundPt, Vector groundn, Vector* groundColorNoObj,
				float* t, double* K, bool* B) {
	
	int j=0, k=0;
	float ndots, ctheta, stheta, rdotv;
	//float rtp=(M_PI*1.0/(sqrtf(ndivs)))*(M_PI*2.0/(sqrtf(ndivs)));
	
	bool aboveground=true, didLightIntersect;
	Ray s;
	
	Vector ld;
	Vector reflection, view;
	
	view=-Normalize(Vector(intersectionPt));
	
	if (t) {
		for (k=0; k<ndivs; k++) {
			t[k]=.0f;
		}
	}
	
	int ishine=0;
	for (ishine=0; ishine<nshines; ishine++) {
		diffuseColors[ishine].x=.0f;
		specularColors[ishine].x=.0f;
		diffuseColors[ishine].y=.0f;
		specularColors[ishine].y=.0f;
		diffuseColors[ishine].z=.0f;
		specularColors[ishine].z=.0f;
		if (groundColorNoObj)
		{
			groundColorNoObj[ishine].x=.0f;
			groundColorNoObj[ishine].y=.0f;
			groundColorNoObj[ishine].z=.0f;
		}
	}
	
	for (j=0; j<ndivs; j++) {
		//         shoot a ray
		
		// you know what? use the light source only if the value of the light source is
		// greater than some epsilon. That will save you a whole ton of time ;)
		//		if ( (diffuseLight[j].x>1e-5 || diffuseLight[j].y>1e-5 || diffuseLight[j].z>1e-5) ||
		//			(hasSpecular && (specularLight[j].x>1e-5 || specularLight[j].y>1e-5 || specularLight[j].z > 1e-5)) ) {
		if ( (diffuseLight[0][j].x>MIN_LIGHT_INTENSITY || diffuseLight[0][j].y>MIN_LIGHT_INTENSITY || diffuseLight[0][j].z>MIN_LIGHT_INTENSITY) ) {
			
            //if (true) {
			//stheta=sinf(Theta[j]);
			//ctheta=cosf(Theta[j]);
			
			stheta=sTheta[j]; ctheta=cTheta[j];
			
			
			//ground no obj only
			// for distant light sources...
#ifdef DISTANT
			s.d.x=stheta*cPhi[j];
			s.d.y=stheta*sPhi[j];
			s.d.z=ctheta;
#else
			// for nearby light sources...
			ld.x=SphereRadius*stheta*cPhi[j]; ld.y=SphereRadius*stheta*sPhi[j]; ld.z=SphereRadius*ctheta; // alternative format where lights are closer
			s.d.x=ld.x-groundPt.x;
			s.d.y=ld.y-groundPt.y;
			s.d.z=ld.z-groundPt.z;
			s.d=Normalize(s.d);
#endif
			
			// ground color without obj...
			s.maxt=INFINITY;
			s.mint=0;
			s.o=groundPt+s.d*(MIN_LIGHT_DIRECTION_ALPHA);
			ndots=Dot(groundn,s.d);
			
			//if (ndots>0) {
            if (true) {
				if (groundColorNoObj) {
					for (ishine=0; ishine<nshines; ishine++) {
						groundColorNoObj[ishine]+=diffuseLight[ishine][j]*fabsf(ndots);
					}
				}
			}
            
			
			// ground and obj
			// for distant light sources...
#ifdef DISTANT
			s.d.x=stheta*cPhi[j];
            s.d.y=stheta*sPhi[j];
            s.d.z=ctheta;
#else
			// for nearby light sources...
			ld.x=SphereRadius*stheta*cPhi[j]; ld.y=SphereRadius*stheta*sPhi[j]; ld.z=SphereRadius*ctheta; // alternative format where lights are closer
			s.d.x=ld.x-intersectionPt.x;
			s.d.y=ld.y-intersectionPt.y;
			s.d.z=ld.z-intersectionPt.z;
			s.d=Normalize(s.d);
#endif
			
			s.maxt=INFINITY;
			s.mint=0;
			s.o=intersectionPt+s.d*(MIN_LIGHT_DIRECTION_ALPHA);
			// first check if the dot product is greater than zero (light source is above primitive
			ndots=Dot(n,s.d);
			//if (ndots>0) {
            if (true) {
				//if (true) {
				// then check if light source is above ground
				
				/*if (didIntersect) {
				 denom=groundAxis[0]*s.d.x+groundAxis[1]*s.d.y+groundAxis[2]*s.d.z;
				 if (fabs(denom)>1e-6) {
				 lambda=-(groundAxis[3]+
				 groundAxis[0]*intersectionPt.x+
				 groundAxis[1]*intersectionPt.y+
				 groundAxis[2]*intersectionPt.z)*1.0f/denom;
				 aboveground=lambda<1e-3;
				 } else aboveground=true;
				 
				 } else aboveground=true;*/
				// only if light source is above ground and above primitive should you
				// consider using it (this check should be done for object pixels not ground pixels)
				//aboveground=ld.x*groundAxis[0]+ld.y*groundAxis[1]+ld.z*groundAxis[2]+groundAxis[3]>0;
				aboveground=true;
				if (aboveground) {
					didLightIntersect=bvhAccel->IntersectP(s);	// COMPUTE INTENSIVE
					if (!didLightIntersect) {
						//         if it does not intersect,
						//				compute the diffuse and specular contributions --- this is something you'll have to adapt
						//              add the contribution to the running sum for the light seen at the pixel
						//color+=diffuseLight[j]*(ndots*rtp*sTheta[j]);
						
						if (t) {
							for (k=0; k<ndivs; k++) {
								if (B[k*ndivs+j]) t[k]+=fabsf(ndots)*K[k*ndivs+j];
							}
						}
						for (ishine=0; ishine<nshines; ishine++) {
							diffuseColors[ishine]+=diffuseLight[ishine][j]*ndots;
						}
						if (hasSpecular) {
							reflection=Normalize(2*fabsf(ndots)*n-s.d);
							rdotv=Dot(reflection,view);
							if (rdotv>0) { // assume ground has no specularity for speed --- not yet
								//specularColor+=specularLight[j]*pow(rdotv, shininess);
								for (ishine=0; ishine<nshines; ishine++) {
									specularColors[ishine]+=specularLight[ishine][j]*(float)exp((double)shines[ishine]*log((double)rdotv));
								}
							}
						}
					}						
				}
			}
			
		}
	}	
}

void quadrature(PbrtPoint intersectionPt, Vector n, Ray r, BVHAccel* bvhAccel,
				Vector** diffuseLight,
				float* cTheta, float* sTheta, float* cPhi, float* sPhi, float SphereRadius,
				int ndivs, float* groundAxis, bool isgnd,
				Vector* diffuseColors,
				PbrtPoint groundPt, Vector groundn, Vector* groundColorNoObj,
				float* t, double* K, bool* B) {
	
	int j=0, k=0;
	float ndots, ctheta, stheta;
	//float rtp=(M_PI*1.0/(sqrtf(ndivs)))*(M_PI*2.0/(sqrtf(ndivs)));
	
	bool aboveground=true, didLightIntersect;
	Ray s;
	
	Vector ld;
	Vector reflection, view;
	
	view=-Normalize(Vector(intersectionPt));
	
	if (t) {
		for (k=0; k<ndivs; k++) {
			t[k]=.0f;
		}
	}
	
	int ishine=0;
	diffuseColors[ishine].x=.0f;
		diffuseColors[ishine].y=.0f;
		diffuseColors[ishine].z=.0f;
		if (groundColorNoObj)
		{
			groundColorNoObj[ishine].x=.0f;
			groundColorNoObj[ishine].y=.0f;
			groundColorNoObj[ishine].z=.0f;
		}
	
	for (j=0; j<ndivs; j++) {
		//         shoot a ray
		
		// you know what? use the light source only if the value of the light source is
		// greater than some epsilon. That will save you a whole ton of time ;)
		//		if ( (diffuseLight[j].x>1e-5 || diffuseLight[j].y>1e-5 || diffuseLight[j].z>1e-5) ||
		//			(hasSpecular && (specularLight[j].x>1e-5 || specularLight[j].y>1e-5 || specularLight[j].z > 1e-5)) ) {
		if ( (diffuseLight[0][j].x>MIN_LIGHT_INTENSITY || diffuseLight[0][j].y>MIN_LIGHT_INTENSITY || diffuseLight[0][j].z>MIN_LIGHT_INTENSITY) ) {
			
            //if (true) {
			//stheta=sinf(Theta[j]);
			//ctheta=cosf(Theta[j]);
			
			stheta=sTheta[j]; ctheta=cTheta[j];
			
			
			//ground no obj only
			// for distant light sources...
#ifdef DISTANT
			s.d.x=stheta*cPhi[j];
			s.d.y=stheta*sPhi[j];
			s.d.z=ctheta;
#else
			// for nearby light sources...
			ld.x=SphereRadius*stheta*cPhi[j]; ld.y=SphereRadius*stheta*sPhi[j]; ld.z=SphereRadius*ctheta; // alternative format where lights are closer
			s.d.x=ld.x-groundPt.x;
			s.d.y=ld.y-groundPt.y;
			s.d.z=ld.z-groundPt.z;
			s.d=Normalize(s.d);
#endif
			
			// ground color without obj...
			s.maxt=INFINITY;
			s.mint=0;
			s.o=groundPt+s.d*(MIN_LIGHT_DIRECTION_ALPHA);
			ndots=Dot(groundn,s.d);
			
			if (ndots>0) { // here we are only measuring the ground, i.e. groundn, so
                // since we always do ndots comparison for the ground, do that here as well
            //if ( !isgnd || (isgnd && ndots>0)) {
            //if (true) {
				if (groundColorNoObj) {
					groundColorNoObj[0]+=diffuseLight[ishine][j]*fabsf(ndots);
				}
			}
            
			
			// ground and obj
			// for distant light sources...
#ifdef DISTANT
			s.d.x=stheta*cPhi[j];
            s.d.y=stheta*sPhi[j];
            s.d.z=ctheta;
#else
			// for nearby light sources...
			ld.x=SphereRadius*stheta*cPhi[j]; ld.y=SphereRadius*stheta*sPhi[j]; ld.z=SphereRadius*ctheta; // alternative format where lights are closer
			s.d.x=ld.x-intersectionPt.x;
			s.d.y=ld.y-intersectionPt.y;
			s.d.z=ld.z-intersectionPt.z;
			s.d=Normalize(s.d);
#endif
			
			s.maxt=INFINITY;
			s.mint=0;
			s.o=intersectionPt+s.d*(MIN_LIGHT_DIRECTION_ALPHA);
			// first check if the dot product is greater than zero (light source is above primitive
			ndots=Dot(n,s.d);
			//if (ndots>0) {
            if ( !isgnd || (isgnd && ndots>0)) {
            //if (true) {
				// then check if light source is above ground
				
				/*if (didIntersect) {
				 denom=groundAxis[0]*s.d.x+groundAxis[1]*s.d.y+groundAxis[2]*s.d.z;
				 if (fabs(denom)>1e-6) {
				 lambda=-(groundAxis[3]+
				 groundAxis[0]*intersectionPt.x+
				 groundAxis[1]*intersectionPt.y+
				 groundAxis[2]*intersectionPt.z)*1.0f/denom;
				 aboveground=lambda<1e-3;
				 } else aboveground=true;
				 
				 } else aboveground=true;*/
				// only if light source is above ground and above primitive should you
				// consider using it (this check should be done for object pixels not ground pixels)
				//aboveground=ld.x*groundAxis[0]+ld.y*groundAxis[1]+ld.z*groundAxis[2]+groundAxis[3]>0;
				aboveground=true;
				if (aboveground) {
					didLightIntersect=bvhAccel->IntersectP(s);	// COMPUTE INTENSIVE
					if (!didLightIntersect) {
						//         if it does not intersect,
						//				compute the diffuse and specular contributions --- this is something you'll have to adapt
						//              add the contribution to the running sum for the light seen at the pixel
						//color+=diffuseLight[j]*(ndots*rtp*sTheta[j]);
						
						if (t) {
							for (k=0; k<ndivs; k++) {
								if (B[k*ndivs+j]) t[k]+=fabsf(ndots)*K[k*ndivs+j];
							}
						}
						diffuseColors[0]+=diffuseLight[0][j]*fabsf(ndots);
					}
				}
			}
			
		}
	}
}

