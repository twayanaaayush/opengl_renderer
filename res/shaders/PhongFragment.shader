#version 330 core

in vec3 FragPos;
in vec3 ec_pos;
in vec3 Normal;
in vec2 TexCoords;
out vec4 FragColor;

struct Material
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Material material;
uniform Light light;
uniform vec3 viewPos;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform bool hasTextures;

void main()
{
    // Use vertex normal if provided, else compute from screen-space derivatives
    vec3 norm;
    if (length(Normal) > 0.001)
        norm = normalize(Normal);
    else
        norm = normalize(cross(dFdx(ec_pos), dFdy(ec_pos)));

    vec3 lightDir = normalize(light.position - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    // Sample colors: texture or material
    vec3 diffuseColor = hasTextures
        ? texture(texture_diffuse1, TexCoords).rgb
        : material.diffuse;

    vec3 specularColor = hasTextures
        ? texture(texture_specular1, TexCoords).rgb
        : material.specular;

    vec3 ambientColor = hasTextures
        ? texture(texture_diffuse1, TexCoords).rgb
        : material.ambient;

    // Phong lighting
    vec3 ambient = light.ambient * ambientColor;
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * diffuseColor;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * specularColor;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
