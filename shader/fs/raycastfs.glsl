#version 330

out vec4 glFragColor;
in vec2 vPosition;

uniform sampler2D raystart;
uniform sampler2D raystop;
uniform sampler3D density;
uniform sampler3D temperature;
uniform int material; //0 smoke, 1 fire, 2 smoke&fire
uniform int width; //window width
uniform int height; //window width

uniform vec3 lightPos = vec3(1.0, 1.0, 2.0);
uniform vec3 lightColor = vec3(10.0);

const float maxLen = sqrt(2.0); //max length of a cube
const int sampleNum = 256;
const int lightsampleNum = 128;
const float stepSize = maxLen/float(sampleNum);
const float lightStepSize = maxLen / float(lightsampleNum);
const float densityScale = 10; //density number is too small to sample
const float lightScale = 0.5; //scale of light color
const float fireColorScale = 20; //scale of fire color
const float absorbRate = 10.0; // light absorption rate by density

void main()
{
    vec2 coord = vec2(gl_FragCoord.x/float(width), gl_FragCoord.y/float(height));
    vec3 enter = texture(raystart, coord).xyz;
    vec3 leave = texture(raystop, coord).xyz;

    if (enter == leave) { discard; return;}

    vec3 ray = leave - enter;
	float raylen = length(ray);
	vec3 step = normalize(ray) * stepSize; //step along the ray

	float alpha = 1.0; //init alpha from eye
	vec3 color = vec3(0);
	glFragColor = vec4(0);
	vec3 pos = enter;


	if(material == 0) //smoke
	{
		while(alpha>0.01 && raylen>0){
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

				alpha *= 1.0-sampleDens*stepSize*absorbRate; //alpha of current color can reach the eye
				color += finallightColor*alpha*sampleDens*stepSize; //sample color contribution
			}

			pos += step;
			raylen -= stepSize;
		}

		glFragColor.rgb = color;
		glFragColor.a = 1-alpha;
	}
	

	if(material == 1) //fire
	{
		bool fullColor = false;
		while(alpha>0.01 && raylen>0){
			float sampleDens = texture(density, pos).x * densityScale; //density is too small, but temprature starts very high
			float T = texture(temperature, pos).x;
			if(sampleDens>0 && T>0){
				//get temprature color
				float thresholdOne = 2.0;
				float thresholdTwo = 5.0;
				vec3 fireColor = vec3(1.0, 1.0, 0.5);
				if(T<thresholdOne) fireColor = vec3(1.0, 0.0, 0.0);
				if(T>=thresholdOne&&T<thresholdTwo) fireColor = vec3(1.0, 1.0, 0.0);

				alpha *= 1.0-sampleDens*stepSize*absorbRate; //alpha of current color can reach the eye

				color += (fireColor*fireColorScale)*alpha*sampleDens*stepSize; //sample color contribution
			}

			if(color == vec3(1.0)){fullColor=true;break;} 
			pos += step;
			raylen -= stepSize;
		}

		glFragColor.rgb = color;
		glFragColor.a = 1-alpha;
	}


	if(material == 2) //smoke & fire
	{
		while(alpha>0.01 && raylen>0){
			float sampleDens = texture(density, pos).x * densityScale; //density is too small, but temprature starts very high
			float T = texture(temperature, pos).x;
			if(sampleDens>0 && T>0){
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

				//get temprature color
				float thresholdOne = 2.0; //red color
				float thresholdTwo = 5.0; //yellow color
				float thresholdSmoke = 1.0; //smoke
				vec3 fireColor = vec3(1.0, 1.0, 0.5);
				if(T<thresholdSmoke) { fireColor = vec3(1.0, 1.0, 1.0) * 0.5 * finallightColor * 0.15; }//apply smoke color on it
				if(T>=thresholdSmoke&&T<thresholdOne) fireColor = vec3(1.0, 0.0, 0.0);
				if(T>=thresholdOne&&T<thresholdTwo) fireColor = vec3(1.0, 1.0, 0.0);

				alpha *= 1.0-sampleDens*stepSize*absorbRate; //alpha of current color can reach the eye
				color += (fireColor*fireColorScale)*alpha*sampleDens*stepSize; //sample color contribution
			}

			pos += step;
			raylen -= stepSize;
		}

		glFragColor.rgb = color;
		glFragColor.a = 1-alpha;
	}


}
