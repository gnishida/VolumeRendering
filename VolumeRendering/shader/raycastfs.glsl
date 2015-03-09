#version 330

in vec3 vPosition;
out vec4 glFragColor;

uniform sampler3D density;
uniform vec3 cameraPos;

uniform vec3 lightPos = vec3(1.0, 1.0, 2.0);

const int lightsampleNum = 128;
const float stepSize = 0.005;
const float lightStepSize = 0.01;
const float densityScale = 10;
const float absorbRate = 10.0;

void main() {
	if (gl_FrontFacing) {
		discard;
		return;
	}

	vec3 eye = (cameraPos + vec3(1,1,1)) * 0.5;
    vec3 obj = (vPosition + vec3(1,1,1)) * 0.5;
	vec3 ray = obj - eye;

	int numSteps = int(length(ray) / stepSize);

	vec3 step = normalize(ray) * stepSize; //step along the ray

	float alpha = 0.0; //init alpha from eye
	vec3 color = vec3(0);
	glFragColor = vec4(0);
	vec3 pos = eye;

	bool outside = true;
	if (pos.x >= 0 && pos.x <= 1 && pos.y >= 0 && pos.y <= 1 && pos.z >= 0 && pos.z <= 1) {
		outside = false;
	}

	for (int i = 0; i < numSteps && alpha < 0.99; ++i) {
		if (outside) {
			if (pos.x < 0 || pos.x > 1 || pos.y < 0 || pos.y > 1 || pos.z < 0 || pos.z > 1) {
				pos += step;
				continue;
			} else {
				outside = false;
			}
		}

		if (pos.x < 0 || pos.x > 1 || pos.y < 0 || pos.y > 1 || pos.z < 0 || pos.z > 1) {
			break;
		}

		float sampleDens = texture(density, pos).x * densityScale;
		if (sampleDens > 1e-5) {
			//get lights color on the pixel
			vec3 lightDir = normalize(lightPos-pos)*lightStepSize;
			vec3 lpos = pos + lightDir;

			//get alpha of how many light can reach the pixel
			float lapha = 1.0;
			for (int s=0; s < lightsampleNum; ++s) {
				float ldens = texture(density, lpos).x;
				lapha *= 1.0-absorbRate*stepSize*ldens; 
				if (lapha <= 0.01) {
					break;
				}
				lpos += lightDir;
			}
			vec3 finallightColor = vec3(10.0) * lapha;

			// alpha blending
			alpha += (1.0 - alpha) * sampleDens*stepSize*absorbRate;
			color += (1.0 - alpha) * sampleDens*stepSize*finallightColor;
		}

		pos += step;
	}

	glFragColor.rgb = color;
	glFragColor.a = alpha;
}
