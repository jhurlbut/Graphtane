// Copyright (c) 2013-2014 Matthew Paul Reid

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef DEEP_SHADOW_MAPS_H
#define DEEP_SHADOW_MAPS_H

struct DeepShadowMapSample
{
	float startDepth;
	float endDepth;
	float endVisibility;
};

DeepShadowMapSample getDeepShadowMapSample(sampler2D shadowSampler, vec3 shadowTexcoord)
{
	vec4 s = texture(shadowSampler, shadowTexcoord.xy);
	
	DeepShadowMapSample shadowSamp;
	shadowSamp.startDepth = s.r;
	shadowSamp.endDepth = s.g + 0.00001; // add bias to ensure endDepth > startDepth
	shadowSamp.endVisibility = s.b;
	return shadowSamp;
}

float getVisibility_dsm(sampler2D shadowSampler, vec3 shadowTexcoord)
{
	DeepShadowMapSample shadowSamp = getDeepShadowMapSample(shadowSampler, shadowTexcoord);
	float receiverDepth = shadowTexcoord.z - 0.00005; // add small bias to prevent self shadowing on when visibility falloff rapid
	
	float startToEndFactor = min(max(receiverDepth - shadowSamp.startDepth, 0.0) / (shadowSamp.endDepth - shadowSamp.startDepth), 1.0);
	return mix(1, shadowSamp.endVisibility, startToEndFactor);
}

#endif