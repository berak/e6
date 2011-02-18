float4 fTime		: register(c1);
sampler samp0		: register(s0);




// All data of our world
float3 spherePos, lightDir, lightColor, waterColor, ro, rd, interlacing;
float sphereRadius, gf_DetailLevel, pi, eps, bigeps;



// Pseudo random number base generator (credits go to iq/rgba)
float rnd(float2 x)
{
	int n = int(x.x * 40.0 + x.y * 6400.0);
	n = (n << 13) ^ n;
	return 1.0 - float( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0;
}


// Generate cubic interpolated random values
float smoothrnd(float2 x)
{
	x = mod(x,1000.0);
    float2 a = fract(x);
    x -= a;
    float2 u = a*a*(3.0-2.0*a);
    return mix(
	mix(rnd(x+float2(0.0,0.0)),rnd(x+float2(1.0,0.0)), u.x),
	mix(rnd(x+float2(0.0,1.0)),rnd(x+float2(1.0,1.0)), u.x), u.y);
}


// Convert the cipher range from [-1,1] to [0,1]
float norm(float x)
{
	return x * 0.5 + 0.5;
}


// Generate animated (t) caustic values
float caustic(float u, float v, float t)
{
	float a = (
	norm(sin(pi * 2.0 * (u + v + Y.y*t))) +
	norm(sin(pi     * (v - u - Y.y*t))) +
	norm(sin(pi     * (v     + Y.y*t))) +
	norm(sin(pi * 3.0 * (u     - Y.y*t)))) * 0.3;
	return pow(a, 2.0);
}


// Calculate our TV effects (interlacing, RGB mask and film grain)
float3 pp(float3 color)
{
	int c = int(mod(gl_FragCoord.x, 3.0));
	if (c==0) color *= interlacing.xyz;
	if (c==1) color *= interlacing.yzx;
	if (c==2) color *= interlacing.zxy;
	return mix(color, float3(norm(smoothrnd(p * 333.0 + rnd(float2(Y.y)) * 33333.0))), Y.x * 0.3 + 0.03);
}


// Our fake godray effect (bad if moving fast, but awesome any other time)
float3 godrays(float3 color)
{
	float2 dpos = p*2.0-1.0;
	float g = dpos.x * (dpos.y +  3.0);
	return color + lightColor * 
		caustic(g + 50.0 * ro.x, g + 50.0 * ro.z, 1.5) * 
		(norm(dpos.y)) * min(-ro.y * 30.0, 0.3);
}



// Our heightmap calculation function, we could use some perlin noise here if it wouldn't be so performance killing

float height(float2 x)
{
	return (-0.035 + pow((caustic(x.x * 10.0, x.y * 10.0, 0.0) * 2.0 - 1.0), 2.0) * 0.05) 
		- (x.x - 0.1) * 0.2; // This line creates one entire continent and a big ocean!
}


// Gets the terrain normal
float3 getTerrainNormal(float3 p)
{
    return normalize(float3(
		height(p.xz - float2(bigeps, 0.0)) - height(p.xz + float2(bigeps, 0.0)),
        2.0 * bigeps,
        height(p.xz - float2(0.0, bigeps)) - height(p.xz + float2(0.0, bigeps))));
}


// Global diffuse lighting formula
float3 diffuseLight(float3 incolor, float3 normal)
{
	return (0.3 + 0.7 * max(dot(normal, lightDir), 0.0)) * lightColor * incolor;
}


// Calculates the water "waves". To reduce the bumpiness, increment the y-axis
float3 getWaterNormal(float3 p)
{
	return normalize(float3(
		caustic(p.x * 160.0 - cos(p.z * 10.0) * 12.0, p.z * 140.0, 4.0),
		8.0,
		caustic(p.z * 160.0 - sin(p.x * 10.0) * 12.0, p.x * 140.0, 4.0)) * 2.0 - 1.0);
}

// Calculate the terrain color for the given voxel
float3 shadeTerrain(float3 p, float3 rd)
{
	float3 n = getTerrainNormal(p);
	float3 color = mix(
			// sandy color
		float3(0.66, 0.55, 0.4)
			
			// basic color (big random color spots)
		- 0.2 * smoothrnd(abs(p.xz * 150.0))
			
			// texture (sediment lines)
		- 0.2 * smoothrnd(abs(p.yy + 0.002 * smoothrnd(abs(p.xz * 150.0))) * 3000.0),
		
			// interleaved grass, hight dependant
		float3(0.1, 0.3, 0) * (smoothrnd(p.xz * 7000.0) * 0.4 + 0.5),
		
			// mixing for the sand/grass transition
		clamp(n.y * (caustic(p.x * 111.0, p.z * 111.0, 0.0) * 0.5 - p.y * 40.0), 0.0, 1.0));

	// caustics, only underwater (no cloudshadows, though)
	if (p.y <= 0.0)
		color += 5.0 * getWaterNormal(0.8 * p).x * min(0.3, -p.y * 8.0);
	
	// Light
	return diffuseLight(color, n);
}


// Create a blueish sky transition from navy blue to badass dark blue
float3 shadeSky(float3 ro, float3 rd)
{
	return ro.y <= -eps*eps ?
		waterColor : 
		(1.0 + Y.z * 0.3) * mix(float3(-0.5, -0.25, 0.0), float3(2.0), 1.0 - (rd.y * 0.5 + 0.5));
}


// Calculates the refraction and reflection of the water surface.
// Also mixes both values by the depth of the water and the fresnel term.
// Possible improvements: fix fake underwater reflection and refraction
float3 shadeWaterRefl(float3 p, float3 newrd)
{
	float3 waterNormal = getWaterNormal(p);
	
	// perform raytracing/raymarching for both reflection and refraction
	// calc the water refraction, the refraction index (0.9) will decrease with the distance to allow a better over/under water transition
	float4 refracted = traceRay(p, refract(newrd, waterNormal, 0.9), 2);//mix(0.9, 1.0, smoothstep(0.01, 0.0, length(p-ro)))), 2);
	

	// calculate the depth factor (water entry point to terrain voxel) (black magic involved here!)
	float depth = clamp(pow(1.03 * (1.0 - length(refracted.xyz - p)), 16.0), 0.0, 1.0);
	
	// Finally stir the pot =)
	return mix(
		ro.y < 0.0 ? shadeSky(p, newrd) : waterColor, // Water color 
		mix(
			shade(traceRay(p, reflect(newrd, waterNormal), 2), p, newrd), // Reflection color 
			shade(refracted, p, newrd), // Refraction color 
			clamp(-rd.y + depth, 0.0, 1.0)), // fresnel term
		refracted.w == 3.0 ? 0.5 : pow(depth, 0.5)); // water color contribution

}



// Texture our "AMIGAAAAAAA!!" ball
float3 shadeAttractor(float3 p, float3 rd)
{
	float3 n,color;
	
	// get the sphere normal first
	n = normalize(p - spherePos);
	
	// now calculate the texture coordinates
	float2 uv = 0.5 + 0.5 * float2(atan(n.z, n.x), acos(n.y)) / pi;
	
	// We'll animate our x-texture coordinate with the time, this gives the impression of a rotating ball
	uv.x -= Y.y;
	
	// This spell will convert any dull ball into an amiga ball, caution is advised.
	color = mix(float3(1), float3(1.0, 0.0, 0.0), mod(step(fract(uv.x * 6.0), 0.5) + step(fract(uv.y * 6.0), 0.5), 2.0));
		
	return diffuseLight(color, n)
		+ pow(max(dot(n, normalize(lightDir - rd)), 0.0), 33.0) * lightColor; // specular light spot
}


// Raymarch the terrain function, returns the distance from the ray origin to the terrain voxel
// This function was originally adopted from an implementation by iq/rgba
float traceTerrain(float3 ro, float3 rd, float maxt)
{
    float delt, lh, ly, samplePosY;
    delt = 0.0; // If the world would consist of only nVidia GPUs, this line wouldn't exist.
    float3 samplePos = ro;

    // advance our sample position from our nearplane to our farplane
    for (float t = 0.0; t < maxt; t += delt)
    {
		// advance our ray
        samplePos += rd * delt;
		samplePosY = samplePos.y;

        // get the height at the given sample 2d (!) position (we could enhance this by sampling a voxel and returning only the distance to the voxel)

        float h = height(samplePos.xz);

        if (samplePosY <= h)
        {
			// we need to know our improved (more accuracy here) real terrainposition and the old sampleposition
			// also we precalculate the traveled ray distance (its not a ray anymore if we use stuff like refraction, eg but hey lets stick to this word)
			return t - delt + delt*(lh-ly)/(samplePosY-h+lh-ly);
        }
        
        // store our last height and last sampleposition on the y-axis
        // we need this to calculate the improved terrainposition which will give us a smoother transition between our samplesteps (rd*delt)
		lh = h;
		ly = samplePosY;
 
		// advance our steplength the more we travel the bigger our stepsize should be
		// with this we are able to sample finer details near to our camera
        delt = 0.002 + (t/gf_DetailLevel);
    }
    
    // we hit nothing
    return 9.0;
}



// Ray vs. sphere intersection function
float traceAttractor(float3 ro, float3 rd)
{
	float3 dst = ro - spherePos;
	float B,D;
	B = dot(dst, rd);
	if (B > 0.0)
		return 9.0;
	D = B*B - dot(dst, dst) + sphereRadius*sphereRadius;
	if (D > 0.0)
	{
		return -B - sqrt(D);
	}
	return 9.0;
}


// Ray vs. plane intersection function
float traceWater(float3 ro, float3 rd)
{
	float tPlane = -ro.y / rd.y;
	return tPlane >= eps ? tPlane : 9.0;
}


// Raytracing entry point, returns voxel and object ID
// IDs:
// 0 = sky (not the armageddon, xTr1m!!)
// 1 = terrain
// 2 = water
// 3 = attractive amiga ball (you have never seen such a sexy amiga ball before, admit it!)
float4 traceRay(float3 ro, float3 rd, int ignore)
{	
	float water, attractor, terrain, minDist;

	// trace only the objects we need (only one could maximally be ignored)
	water = ignore != 2 ? traceWater(ro, rd) : 9.0;
	attractor = ignore != 3 ? traceAttractor(ro, rd) : 9.0;
	terrain = ignore != 1 ? traceTerrain(ro, rd, min(0.5, 0.002+min(water, attractor))) : 9.0;
	
	// auto detail level reducing (common dude, give the GPU some breathing room)
	gf_DetailLevel /= 20.0;
	
	// find the nearest distance
	minDist = min(terrain, min(water, min(attractor, 9.0)));
	
	// we hit nothing or the hitpoint is too far
	if (minDist == 9.0)
		return float4(0.0);

	// calculate the hit/voxel position
	float3 hitPos = ro + rd * minDist;

	// check what we might have hit
	if (minDist == terrain) 
		return float4(hitPos, 1.0);
	if (minDist == water) 
		return float4(hitPos, 2.0); 
	if (minDist == attractor) 
		return float4(hitPos, 3.0);

	// Panic, worry, die to death! Probably we'll land on the moon (this should never happen)
	//return float4(0);
}


// Entrypoint for color calculation
float3 shadeRefl(float4 hitPoint, float3 newRo, float3 rd)
{
	// determine the fog color for this very precise point in the space time continuum
	float3 myFog = newRo.y < eps ? waterColor : shadeSky(ro, rd);
	
	// generate the distance value for the fog calculation
	float distance = clamp(length(hitPoint.xyz - newRo) * (ro.y <= 0.0 ? 4.0 : 2.0), 0.0, 1.0);

	// get the color of the hit object and mix it with the fog
	// in most cases we allow further raytracing here (not for the terrain, its not shiny enough)
	if (hitPoint.w == 1.0)
		return mix(shadeTerrain(hitPoint.xyz, rd), myFog, distance);
	if (hitPoint.w == 2.0)
		return mix(shadeWaterRefl(hitPoint.xyz, rd), myFog, distance);
	if (hitPoint.w == 3.0)
		return mix(
			// Our amiga ball is shiny so reflect the scene!
			mix(shadeAttractor(hitPoint.xyz, rd), shade(traceRay(hitPoint.xyz, reflect(rd, normalize(hitPoint.xyz - spherePos)), 3), hitPoint.xyz, rd), 0.5)
			, myFog, distance);
		
	return shadeSky(newRo, rd);
}


// Get the color from the object we just hit (without further raytraces)
// this is necessary because no recursion is allowed in GLSL (damn you!)
float3 shade(float4 hitPoint, float3 newRo, float3 rd)
{
	// determine the fog color for the very same point we discussed earlier
	float3 myFog = newRo.y < eps ? waterColor : shadeSky(ro, rd);
	
	// generate the other distance value. Paid attention? If you don't know what value I'm talking about, rtfm or gtfo.
	float distance = clamp(length(hitPoint.xyz - newRo) * (ro.y <= 0.0 ? 4.0 : 2.0), 0.0, 1.0);

	// get the color of the hit object and mix it with the fog
	if (hitPoint.w == 1.0)
		return mix(shadeTerrain(hitPoint.xyz, rd), myFog, distance);
	if (hitPoint.w == 2.0)
		return mix(waterColor, myFog, distance);
	if (hitPoint.w == 3.0)
		return mix(shadeAttractor(hitPoint.xyz, rd), myFog, distance);
		
	return myFog;
}


void PS(
	in  float3 pos  : TEXCOORD0,
	out float4 col  : COLOR
)
{
    // x: Noise intensity
    // y: #sceneid.#scenetime (float)
    // z: Snare drum intensity (amiga ball radius gain)
    // w: Aspect ratio
    float4 Y;
    Y.x = 0.5;
    Y.y = (fmod(fTime.x,136.0)+5.0)*71.0/240.0;
    Y.z = 1.0;
    Y.w = 1.333;

    

    float angle = 1.570796*Y.y - 1.570796;
    float4x4 mm = float4x4( cos(angle), 0.0, sin(angle), 0.0,
                   0.0,       1.0, 0.0,       0.0,
                   -sin(angle), 0.0, cos(angle), 0.0,
                   0.0,       0.0, 0.0,       1.0 );

	// Set the quality setting for the raymarcher, a higher value results in a longer processing time
	// try to find a good balance between these two, low values will result in a wobbling endresult
	// low quality = 50.0 (visual results are ok at 640x480)
	// mid quality = 100.0
	// high quality = 200.0
	gf_DetailLevel = 100.0;

	// Give our saviour global variables some life!
	pi = 3.1416;
	interlacing = float3(1.2, 0.9, 0.9);
	eps = 0.0001;
	bigeps = 0.01;

	// Nifty random number generator gets initialized
	float seed = 10.0;

	// Determine the scene we're in
	int scene = int(Y.y);

	// Get the look direction for the current pixel (always look forwards)
	rd = float3((p.xy - 0.5), 1.0);

	// Merry-go-round on a boat (yeah, this makes no sense. Go watch the intro and see for yourself)
	if (scene > 22 && scene < 27)
	{
		seed = min(1.0, sin((Y.y-23.0)*pi*0.25)*12.0);
		ro = float3(0.12, 0.005, Y.y*0.08);
		rd = float3(mm * float4(rd, 1.0));
		rd.y += 0.1*cos(Y.y*4.0);
	}
	// Intermezzo: Dolphin like animation inside and outside the water, chasing that amiga ball!
	else if (scene > 14 && scene < 23)
	{
		seed = min(1.0, sin((Y.y-15.0)*pi*0.125)*24.0);
		rd += float3(0,0.1*cos(Y.y*4.0), 0.0);
		ro = float3(0.08, 0.01*sin(Y.y*4.0)+0.002, Y.y*0.11);
	}
	// Intro and Outro: Show still scenes
	else
	{
		// Get a random initial position for our camera
		ro = float3(0.1,0.004,0.0) + float3(0.1,0.005,20.0)

			              *float3(rnd(float2(scene, seed++)), rnd(float2(scene, seed++)), rnd(float2(scene, seed++)));
		

		// Basing on the initial position, choose some "random" start and end points nearby
		ro = mix(

			ro+float3(0.008)*float3(rnd(float2(scene, seed++)), rnd(float2(scene, seed++)), rnd(float2(scene, seed++))),

			ro+float3(0.008)*float3(rnd(float2(scene, seed++)), rnd(float2(scene, seed++)), rnd(float2(scene, seed++))),

			// and move the camera!
			Y.y-float(scene));
		
		// We adjust the height of the camera to the terrain height
		ro.y += height(ro.xz)+0.02;
		
		// Deviate the camera position in the direction of the normal of the underlying terrain 
		ro += 0.02*getTerrainNormal(ro);
		
		// Reusing a float variable here, this controls the scene fade in / fade out animation
		seed = min(1.0, step(-28.0, -Y.y) * sin((Y.y-float(scene))*pi)*3.0);
	}
	
	rd = normalize(rd);

	// Now boot the amiga workbench (erm, no...)
	// mantain a relative distance to the camera
 	if (scene > 22 && scene < 27)
		spherePos = ro + 0.1 * float3(mm * float4(0.0, 0.0, 1.0, 1.0));
	else
		spherePos = ro + 0.02 * float3(sin(Y.y), 0.0, 5.0+cos(Y.y));
		
	spherePos.y += 0.01 + height(spherePos.xz); // mantain a relative height to the underlying terrain

	sphereRadius = scene < 14 ? 0.0 : bigeps * 0.5 + bigeps * Y.z; // The amiga ball is bigger when the snare drum is hit!

	spherePos += 2.0 * bigeps * getTerrainNormal(spherePos); // deviate according to the underlying terrain's normal

	

	// Make our world pretty and worthy to live in (you can cultivate algae and eat them, they're surely enough for survival)

	lightDir = float3(0.58, 0.58, -0.58);
	lightColor = float3(1.2) + Y.z;
	waterColor = (1.0 + Y.z * 0.3) * float3(0.3, 0.33, 0.4);
	
	// Our GPU feels good underwater, almost like a refreshing experience :) cool, eh?
	if (ro.y <= 0.0)
	{
		// Less work to do...
		gf_DetailLevel *= 0.75;
		
		// ...and a cozy darker atmosphere
		lightColor *= 0.8;
	}
	
	// Here we go, shoot'em rays and get the color of our fragment!
	float3 color = shadeRefl(traceRay(ro, rd, 0), ro, rd);
	
	// Underwater there are beams of light emanating from god (so called "god" rays...)
	// ...this prooves that god is nothing less than a water surface.
	if (ro.y <= 0.0)
		color = godrays(color);
		
	// Apply post processing and fade effects to the color, and finally return it.
	gl_FragColor.xyz = pp(step(2.0, Y.y) * seed * color);
	gl_FragColor.w = 1.0;
}
