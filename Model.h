#ifndef MODEL_H
#define MODEL_H

#include "Framework.h"
#include "Shader.h"
#include "Framebuffer.h"

enum RenderPass {
	NORMALS_PASS,
	FINAL_PASS,
};

class Model {
	public:
		Model();
		~Model();

		void loadFromFile(const std::string &dir, const std::string &filename, Assimp::Importer &importer);
		void useShader(Shader *shader);

		void setTransformation(aiMatrix4x4 &t);

		void render(RenderPass pass, Framebuffer *normalsBuffer);

	private:
		static sf::Image white;

		aiMatrix4x4 transformation;
		const aiScene *scene;

		Shader *shader;
		sf::Image *diffuse, *specular;

		unsigned *indexBuffer;
};

#endif
