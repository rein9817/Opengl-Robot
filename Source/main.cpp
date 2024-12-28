#include "../Include/Common.h"
#include <iostream>

//For GLUT to handle 
#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3

//GLUT timer variable
float timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;
bool timerRunning = false;
unsigned int lastTime = 0;


bool rotationMode = false;
bool translationMode = false;


float cameraRadius = 10.0f;
float cameraPhi = 0.0f;   
float cameraTheta = 0.0f; 
int lastMouseX = 0;
int lastMouseY = 0;

glm::mat4 robotRotation = glm::mat4(1.0f);
float lastX = 400, lastY = 300;
bool firstMouse = true;


glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
using namespace glm;
using namespace std;

mat4 view(1.0f);			// V of MVP, viewing matrix
mat4 projection(1.0f);		// P of MVP, projection matrix
mat4 model(1.0f);			// M of MVP, model matrix
vec3 temp = vec3();			// a 3 dimension vector which represents how far did the ladybug should move

GLint um4p;
GLint um4mv;

GLuint program;			// shader program id

typedef struct
{
	GLuint vao;			// vertex array object
	GLuint vbo;			// vertex buffer object

	int materialId;
	int vertexCount;
	GLuint m_texture;
} Shape;

Shape head, body, left_arm1, left_arm2, right_arm1, right_arm2,left_leg1,left_leg2,right_leg1,right_leg2;

struct Node
{
	Shape* shape;
	glm::mat4 localTransform;
	std::vector<Node*> children;
};

Node* root = nullptr;


float cameraSpeed = 0.5f;
float zoomFactor = 45.0f;
glm::vec3 cameraPos = glm::vec3(0.0f, 1.5f, 10.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); 
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

glm::mat4 rotationMatrix;
bool isWalking = false;
float walkSpeed = 0.5f;
float walkCycle = 0.0f;
bool flag = true;

// Load shader file to program
char** loadShaderSource(const char* file)
{
	FILE* fp = fopen(file, "rb");
	fseek(fp, 0, SEEK_END);
	long sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *src = new char[sz + 1];
	fread(src, sizeof(char), sz, fp);
	src[sz] = '\0';
	char **srcp = new char*[1];
	srcp[0] = src;
	return srcp;
}

// Free shader file
void freeShaderSource(char** srcp)
{
	delete srcp[0];
	delete srcp;
}

// Load .obj model
void loadSingleModel(Shape& shape, const char* filename, glm::vec3 scale=glm::vec3(1.0f))
{
	tinyobj::attrib_t attrib;
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string warn, err;
	string texture_filename="red.png";



	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename);

	vector<float> vertices, texcoords, normals;
	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			vertices.push_back(attrib.vertices[3 * index.vertex_index + 0]);
			vertices.push_back(attrib.vertices[3 * index.vertex_index + 1]);
			vertices.push_back(attrib.vertices[3 * index.vertex_index + 2]);

			if (index.texcoord_index >= 0) {
				texcoords.push_back(attrib.texcoords[2 * index.texcoord_index + 0]);
				texcoords.push_back(attrib.texcoords[2 * index.texcoord_index + 1]);
			}

			if (index.normal_index >= 0) {
				normals.push_back(attrib.normals[3 * index.normal_index + 0]);
				normals.push_back(attrib.normals[3 * index.normal_index + 1]);
				normals.push_back(attrib.normals[3 * index.normal_index + 2]);
			}
		}
	}


	for (size_t i = 0; i < vertices.size(); i++)
	{
		vertices[i] *= scale.x;
		vertices[i + 1] *= scale.y;
		vertices[i + 2] *= scale.z;
	}
	shape.vertexCount = vertices.size() / 3;

	glGenVertexArrays(1, &shape.vao);
	glBindVertexArray(shape.vao);

	glGenBuffers(1, &shape.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, shape.vbo);

	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float) + texcoords.size() * sizeof(float) + normals.size() * sizeof(float), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), texcoords.size() * sizeof(float), texcoords.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float) + texcoords.size() * sizeof(float), normals.size() * sizeof(float), normals.data());

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(vertices.size() * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(vertices.size() * sizeof(float) + texcoords.size() * sizeof(float)));
	glEnableVertexAttribArray(2);

	texture_data tdata = loadImg(texture_filename.c_str());

	glGenTextures(1, &shape.m_texture);
	glBindTexture(GL_TEXTURE_2D, shape.m_texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	delete[] tdata.data;

	cout << "Loaded " << shape.vertexCount << " vertices for " << filename << endl;
}


void My_LoadModels()
{
	loadSingleModel(head, "Sphere.obj", glm::vec3(1.0f));
	
	loadSingleModel(body, "cube.obj", glm::vec3(1.5f, 1.2f, 1.0f)); 

	loadSingleModel(left_arm1, "Cube.obj", glm::vec3(1.4f, 0.8f, 1.0f));
	loadSingleModel(left_arm2, "Cube.obj", glm::vec3(1.0f));
	loadSingleModel(right_arm1, "Cube.obj", glm::vec3(1.4f, 0.8f, 1.0f));
	loadSingleModel(right_arm2, "Cube.obj", glm::vec3(1.0f));
	loadSingleModel(left_leg1, "Cube.obj", glm::vec3(1.0f));
	loadSingleModel(left_leg2, "Cube.obj", glm::vec3(1.0f));
	loadSingleModel(right_leg1, "Cube.obj", glm::vec3(1.0f));
	loadSingleModel(right_leg2, "Cube.obj", glm::vec3(1.0f));
}

void UpdateCamera() {
	view = lookAt(cameraPos, cameraTarget, cameraUp);
}


void RenderNode(Node* node, const glm::mat4& parentTransform)
{
	glm::mat4 modelMatrix = parentTransform * robotRotation * node->localTransform;

	if (isWalking) {
		if (node->shape == &left_arm1 || node->shape == &right_arm1) {
			float armSwing = sin(walkCycle) * glm::radians(60.0f);
			glm::vec3 rotationAxis(1.0f, 0.0f, 0.0f);
			modelMatrix = glm::rotate(modelMatrix, (node->shape == &left_arm1 ? armSwing : -armSwing), rotationAxis);
		}
		else if (node->shape == &left_leg1 || node->shape == &right_leg1) {
			float legSwing = sin(walkCycle) * glm::radians(45.0f);
			glm::vec3 rotationAxis(1.0f, 0.0f, 0.0f);
			modelMatrix = glm::rotate(modelMatrix, (node->shape == &left_leg1 ? -legSwing : legSwing), rotationAxis);
		}
		else if (node->shape == &head) {
			float headSwing = sin(walkCycle * 2) * glm::radians(15.0f);
			modelMatrix = glm::rotate(modelMatrix, headSwing, glm::vec3(0.0f, 1.0f, 0.0f));
		}
	}

	glUseProgram(program);
	glUniformMatrix4fv(um4p, 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, glm::value_ptr(view * modelMatrix));

	glBindVertexArray(node->shape->vao);
	glDrawArrays(GL_TRIANGLES, 0, node->shape->vertexCount);

	for (Node* child : node->children)
	{
		RenderNode(child, modelMatrix);
	}
}

// OpenGL initialization
void My_Init()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	program = glCreateProgram();

	char** vertexShaderSource = loadShaderSource("vertex.vs.glsl");
	char** fragmentShaderSource = loadShaderSource("fragment.fs.glsl");
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
	glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);
	freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource);
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	um4p = glGetUniformLocation(program, "um4p");
	um4mv = glGetUniformLocation(program, "um4mv");

	glUseProgram(program);

	My_LoadModels();

	root = new Node{ &body, glm::mat4(1.0f), {} };

	Node* headNode = new Node{ &head, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.3f, 0.0f)), {} };

	Node* leftArm1Node = new Node{ &left_arm1, glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, 1.0f, 0.0f)), {} };
	Node* rightArm1Node = new Node{ &right_arm1, glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 1.0f, 0.0f)), {} };

	Node* leftArm2Node = new Node{ &left_arm2, glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.8f, 0.0f)), glm::vec3(0.5f, 1.0f, 0.5f)), {} };
	Node* rightArm2Node = new Node{ &right_arm2, glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.8f, 0.0f)), glm::vec3(0.5f, 1.0f, 0.5f)), {} };

	Node* leftLeg1Node = new Node{ &left_leg1, glm::translate(glm::mat4(1.0f), glm::vec3(-0.8f, -1.5f, 0.0f)), {} };
	Node* rightLeg1Node = new Node{ &right_leg1, glm::translate(glm::mat4(1.0f), glm::vec3(0.8f, -1.5f, 0.0f)), {} };

	Node* leftLeg2Node = new Node{ &left_leg2, glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f)), glm::vec3(0.5f, 1.0f, 0.5f)), {} };
	Node* rightLeg2Node = new Node{ &right_leg2, glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f)), glm::vec3(0.5f, 1.0f, 0.5f)), {} };

	root->children.push_back(headNode);
	root->children.push_back(leftArm1Node);
	root->children.push_back(rightArm1Node);
	root->children.push_back(leftLeg1Node);
	root->children.push_back(rightLeg1Node);

	leftArm1Node->children.push_back(leftArm2Node);
	rightArm1Node->children.push_back(rightArm2Node);
	leftLeg1Node->children.push_back(leftLeg2Node);
	rightLeg1Node->children.push_back(rightLeg2Node);
	cameraPos = glm::vec3(0.0f, 0.0f, 10.0f);
	cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	cameraTarget = cameraPos + cameraFront;
	robotRotation = glm::mat4(1.0f);
	UpdateCamera();
}




// GLUT callback. Called to draw the scene.
void My_Display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glm::mat4 rotatedModel = robotRotation;
	RenderNode(root, rotatedModel);

	glutSwapBuffers();
}


// Setting up viewing matrix
void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);

	float viewportAspect = (float)width / (float)height;

	projection = glm::perspective(glm::radians(zoomFactor), viewportAspect, 0.1f, 1000.0f);
	glm::vec3 cameraPos = glm::vec3(0.0f, 3.0f, 10.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

void My_Timer(int val)
{
	if (!timer_enabled)
	{
		timerRunning = false;
		return;
	}

	unsigned int currentTime = glutGet(GLUT_ELAPSED_TIME);
	float deltaTime = (currentTime - lastTime) / 1000.0f;
	lastTime = currentTime;

	if (isWalking)
	{
		walkCycle += walkSpeed * deltaTime;
		if (walkCycle > 2 * glm::pi<float>())
		{
			walkCycle -= 2 * glm::pi<float>();
		}
	}

	glutPostRedisplay();
	if (timer_enabled)
	{
		glutTimerFunc(timer_speed, My_Timer, val);
	}
	else
	{
		timerRunning = false;
	}
}


void My_Keyboard(unsigned char key, int x, int y) {
	float cameraSpeed = 0.5f;
	switch (key) {
	case 'a': // left
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		break;
	case 'd': // right
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		break;
	case 'w': // up
		cameraPos += cameraSpeed * cameraFront;
		break;
	case 's': // down
		cameraPos -= cameraSpeed * cameraFront;
		break;
	case 'r':
	case 'R':
		rotationMode = !rotationMode;
		if (rotationMode) {
			std::cout << "Robot rotation mode enabled. Use mouse to rotate the entire robot." << std::endl;
			glutSetCursor(GLUT_CURSOR_CROSSHAIR);
		}
		else {
			std::cout << "Robot rotation mode disabled." << std::endl;
			glutSetCursor(GLUT_CURSOR_INHERIT);
		}
		break;
	}
	cameraTarget = cameraPos + cameraFront;
	UpdateCamera();
	glutPostRedisplay();
}

void My_MouseMove(int x, int y) {
	if (firstMouse) {
		lastX = x;
		firstMouse = false;
	}

	float xoffset = x - lastX;
	lastX = x;

	if (rotationMode) {
		float sensitivity = 0.005f;
		xoffset *= sensitivity;
		glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), xoffset, glm::vec3(0.0f, 1.0f, 0.0f));
		robotRotation = rotationY * robotRotation;

		glutPostRedisplay();
	}
}




void My_SpecialKeys(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_F1:
		printf("F1 is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_PAGE_UP:
		printf("Page up is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_LEFT:
		printf("Left arrow is pressed at (%d, %d)\n", x, y);
		break;
	default:
		printf("Other special key is pressed at (%d, %d)\n", x, y);
		break;
	}
}

void My_Menu(int id)
{
	
	switch (id)
	{
	case MENU_TIMER_START:
		if (!timerRunning)
		{
			timer_enabled = true;
			isWalking = true;
			timerRunning = true;
			lastTime = glutGet(GLUT_ELAPSED_TIME);
			std::cout << "Animation started. Initial time: " << lastTime << std::endl;
			glutTimerFunc(timer_speed, My_Timer, 0);
		}
		else
		{
			std::cout << "Animation already running." << std::endl;
		}
		break;
	case MENU_TIMER_STOP:
		timer_enabled = false;
		isWalking = false;
		timerRunning = false;
		std::cout << "Animation stopped" << std::endl;
		break;
	case MENU_EXIT:
		exit(0);
		break;
	default:
		break;
	}
}

void My_Mouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		lastMouseX = x;
		lastMouseY = y;
	}
}

void My_MouseWheel(int button, int dir, int x, int y)
{
	if (dir > 0)
	{
		zoomFactor -= 2.0f;
		if (zoomFactor < 1.0f) //zoom up
		{
			zoomFactor = 1.0f; 
		}
	}
	else {
		zoomFactor += 2.0f;  
		if (zoomFactor > 45.0f) {
			zoomFactor = 45.0f;  
		}
	}
	projection = glm::perspective(glm::radians(zoomFactor), (float)600 / (float)600, 0.1f, 100.0f);
	glutPostRedisplay();
}

int main(int argc, char *argv[])
{
#ifdef __APPLE__
	// Change working directory to source code path
	chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(250, 250);
	glutInitWindowSize(800, 800);
	glutCreateWindow("Robot"); 
#ifdef _MSC_VER
	glewInit();
#endif

	dumpInfo();
	My_Init();
	timer_enabled = false; 
	isWalking = false;


	int menu_main = glutCreateMenu(My_Menu);
	glutAddMenuEntry("Start Animation", MENU_TIMER_START);
	glutAddMenuEntry("Stop Animation", MENU_TIMER_STOP);
	glutAddMenuEntry("Exit", MENU_EXIT);
	glutAttachMenu(GLUT_RIGHT_BUTTON);


	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutKeyboardFunc(My_Keyboard);
	glutMotionFunc(My_MouseMove);
	glutSpecialFunc(My_SpecialKeys);
	glutTimerFunc(timer_speed, My_Timer, 0);
	glutMouseFunc(My_Mouse);
	glutMouseWheelFunc(My_MouseWheel);
	glutMainLoop();

	return 0;
}