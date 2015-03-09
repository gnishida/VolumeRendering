#version 330

in vec3 vPosition;
out vec4 glFragColor;

uniform sampler3D density;
uniform vec3 cameraPos;

uniform vec3 lightPos = vec3(1.0, 1.0, 2.0);
uniform vec3 lightColor = vec3(10.0);

const int lightsampleNum = 128;
const float stepSize = 0.005;
const float lightStepSize = 0.01;
const float densityScale = 10; //density number is too small to sample
const float lightScale = 0.5; //scale of light color
const float absorbRate = 10.0; // light absorption rate by density

void main() {
	if (gl_FrontFacing) {
		discard;
		return;
	}

	vec3 enter = (cameraPos + vec3(1,1,1)) * 0.5;
    vec3 leave = (vPosition + vec3(1,1,1)) * 0.5;
	vec3 ray = leave - enter;
	float raylen = length(ray);

	vec3 step = normalize(ray) * stepSize; //step along the ray

	float alpha = 0.0; //init alpha from eye
	vec3 color = vec3(0);
	glFragColor = vec4(0);
	vec3 pos = enter;

	bool outside = true;
	if (enter.x >= 0 && enter.x <= 1 && enter.y >= 0 && enter.y <= 1 && enter.z >= 0 && enter.z <= 1) {
		outside = false;
	}
	while (alpha < 0.99 && raylen > 0) {
		if (outside) {
			if (pos.x < 0 || pos.x > 1 || pos.y < 0 || pos.y > 1 || pos.z < 0 || pos.z > 1) {
				pos += step;
				raylen -= stepSize;
				continue;
			} else {
				outside = false;
			}
		}

		if (pos.x < 0 || pos.x > 1 || pos.y < 0 || pos.y > 1 || pos.z < 0 || pos.z > 1) {
			break;
		}

		float sampleDens = texture(density, pos).x * densityScale; //density is too small, but temprature starts very high
		if(sampleDens>0){
			//get lights color on the pixel
			vec3 lightDir = normalize(lightPos-pos)*lightStepSize;
			vec3 lpos = pos + lightDir;

			//get alpha of how many light can reach the pixel
			float lapha = 1.0;
			for (int s=0; s < lightsampleNum; ++s) {
				float ldens = texture(density, lpos).x;
				lapha *= 1.0-absorbRate*stepSize*ldens; 
				if (lapha <= 0.01) {break;}//if light can't go through
				lpos += lightDir;
			}
			vec3 finallightColor = lightColor*lapha;

			alpha += (1.0 - alpha) * sampleDens*stepSize*absorbRate; //alpha of current color can reach the eye
			color += (1.0 - alpha) * sampleDens*stepSize*finallightColor; //sample color contribution
		}

		pos += step;
		raylen -= stepSize;
	}

	glFragColor.rgb = color;
	glFragColor.a = alpha;
}
