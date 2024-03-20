#include <iostream>
#include <cyTriMesh.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cyGL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void keyboardCallback(unsigned char, int, int);
void mouseCallback(int, int, int, int);
void idleCallback();
void displayCallback();
void specialFuncCallback(int key, int x, int y);

// Default window properties.
unsigned int screenWidth = 800;
unsigned int screenHeight = 800;

// Teapot Properties.
cyTriMesh teapotMesh;
GLuint teapotVao;
cy::GLSLProgram teapotProgram;

// The floor plane.
GLuint floorVAO;
cy::GLSLProgram floorProgram;

// Shadow map properties.
cy::GLRenderDepth2D shadowMap;
cy::GLSLProgram shadowProgram;
unsigned int shadWidth = 800;
unsigned int shadHeight = 800;
// The shadow view matrix has the camera located AT the light source!
auto shadowView = glm::mat4(1.f);
auto shadowProjection = glm::mat4(1.f);

// The light position.
glm::vec3 lightPos = glm::vec3(70, 15, 70);
glm::vec3 lightTarget = glm::vec3(0, 0, 0);
float lightSpread = glm::radians(89.f);

// Mouse callback fields.
int firstX = 0;
int firstY = 0;
// Camera settings.
float cameraxRot = 283;
float camerayRot = 3;
float teapotZoom;

// Methods for loading our objects.
// I.E. filling in their VAOS.
void loadTeapot(char* file);
void loadFloor();

// Passes in the MVP matrices to our objects.
void prepareTeapotMatrices();
void prepareFloorMatrices();
void prepareShadowMatrices();

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Please provide a file name to load!" << std::endl;
        return 1;
    }
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glViewport(0, 0, screenWidth, screenHeight);
    glutInitWindowSize(screenWidth, screenHeight);
    glutCreateWindow("Project 7 Shadow Mapping");
    glutInitContextFlags(GLUT_DEBUG);

    glEnable(GL_DEPTH_TEST);
    // Register the callbacks here.
    glutIdleFunc(idleCallback);
    glutDisplayFunc(displayCallback);
    glutKeyboardUpFunc(keyboardCallback);
    glutMouseFunc(mouseCallback);
    glutSpecialFunc(specialFuncCallback);
    // Initializes every extension!
    // Avoids a ton of complexity!
    glewInit();

    // Build our Teapot's VAOs.
    loadTeapot(argv[1]);
    // We can now calculate a good zoom.
    teapotMesh.ComputeBoundingBox();
    teapotZoom = (teapotMesh.GetBoundMax() - teapotMesh.GetBoundMin()).Length();

    // Build our floor's VAOs.
    loadFloor();

    // ===== SHADOW MAP INIT =====
    shadowMap.Initialize(
            true,
            shadWidth,
            shadHeight
    );
    shadowMap.BuildTextureMipmaps();
    shadowMap.SetTextureFilteringMode(GL_LINEAR, GL_LINEAR);
    shadowView = glm::lookAt(lightPos, lightTarget, glm::vec3(0, 1, 0));
    // We can use a perspective matrix as a spotlight effect!
    // 1000 is some bogus number, lol!
    shadowProjection = glm::perspective(lightSpread, 1.0f, 0.1f, 200.f);
    shadowProgram.BuildFiles("../shadow.vert", "../shadow.frag");

    glutMainLoop();
    return 0;
}

void displayCallback() {
    glClearColor(0.f, 0.2f, 0.2f, 1.f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    // First Pass -> render to depth map.
    shadowMap.Bind();
    shadowProgram.Bind();
    prepareShadowMatrices();
    glClear(GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(teapotVao);
    glDrawArrays(GL_TRIANGLES, 0, teapotMesh.NF() * 3);
    glBindVertexArray(floorVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    shadowMap.Unbind();

//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
//    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Second Pass
    glClearColor(0.f, 0.2f, 0.2f, 1.f);

    // Rerender the teapot.
    // THIS GIVES ME SIMILAR RESULTS!
    //glBindTexture(GL_TEXTURE_2D, 14);
//    shadowMap.BindTexture(0);
    glBindTexture(GL_TEXTURE_2D, shadowMap.GetTextureID());
    teapotProgram.Bind();
    prepareTeapotMatrices();
    glBindVertexArray(teapotVao);
//    glBindTexture(GL_TEXTURE_2D, shadowMap.GetTextureID());
    glDrawArrays(GL_TRIANGLES, 0, teapotMesh.NF() * 3);

    // The floor should be able to use the same shader as the teapot.
//    prepareFloorMatrices();
    glBindVertexArray(floorVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glutSwapBuffers();
}

void idleCallback() {
    // Tell GLUT to redraw.
    glutPostRedisplay();
}

void keyboardCallback(unsigned char key, int x, int y) {
    if (key == 27) {
        glutLeaveMainLoop();
    }
}

/**
 * Handles the functional keys on the keyboard.
 * Like the F6 key.
 * @param key The special key that is pressed.
 * @param x Relative x position of the mouse when key is pressed.
 * @param y Relative y position of the mouse when key is pressed.
 */
void specialFuncCallback(int key, int x, int y) {
    if (key == GLUT_KEY_F6) {
        std::cout << "Rebuilt Shaders!" << std::endl;
        teapotProgram.BuildFiles("../teapot.vert", "../teapot.frag");
        teapotProgram.Bind();
        // Tell GLUT to redraw.
        glutPostRedisplay();
    }
}

void mouseCallback(int button, int state, int x, int y) {
    // Button 0 is left click.
    // Button 2 is right click.
    // State 0 is begin, 1 is release.
    if (state == 0) {
        firstX = x;
        firstY = y;
        return;
    }
    int deltaX = x - firstX;
    int deltaY = y - firstY;

    if (button == 0) {
        cameraxRot += deltaX / 5;
        camerayRot += deltaY / 5;
        std::cout << cameraxRot << ", " << camerayRot << std::endl;
    }
    if (button == 2) {
        teapotZoom += deltaY / 15;

    }
}

void loadTeapot(char* file){
    // Build our teapotProgram.
    // Must be binded before setting uniforms.
    teapotProgram.Bind();
    teapotProgram.BuildFiles("../teapot.vert", "../teapot.frag");

    teapotMesh = cyTriMesh();
    teapotMesh.LoadFromFileObj(file, true);
    // Build the buffer containing all the verts!
    // I did this because I think I had an issue otherwise like 2 months ago lol.
    cy::Vec3f vertices[teapotMesh.NV()];
    for (int i = 0; i < teapotMesh.NV(); i++) {
        vertices[i] = teapotMesh.V(i);
    }
    // Construct an array to contain the vertices of EVERY face.
    // This means that each vertex will appear multiple times!
    cy::Vec3f positions[teapotMesh.NF() * 3];
    for (int i = 0; i < teapotMesh.NF(); i++) {
        int offset = i * 3;
        for (int j = 0; j < 3; j++) {
            positions[offset + j] = vertices[teapotMesh.F(i).v[j]];
        }
    }
    cy::Vec3f normals[teapotMesh.NF()*3];
    for(int i = 0; i < teapotMesh.NF(); i++){
        int offset = i*3;
        for(int j = 0; j < 3; j++){
            normals[offset+j] = teapotMesh.VN(teapotMesh.FN(i).v[j]);
        }
    }
    // The teapot's VAO.
    glCreateVertexArrays(1, &teapotVao);
    glBindVertexArray(teapotVao);
    // Build the VBOs.
    GLuint vertBuffer;
    glGenBuffers(1, &vertBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    GLuint pos = glGetAttribLocation(teapotProgram.GetID(), "pos");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *) 0);

    GLuint normBuffer;
    glGenBuffers(1, &normBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);
    GLuint norm = glGetAttribLocation(teapotProgram.GetID(), "norm");
    glEnableVertexAttribArray(norm);
    glVertexAttribPointer(norm, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *) 0);
}

void loadFloor(){

    glGenVertexArrays(1, &floorVAO);
    glBindVertexArray(floorVAO);

    float floorVertices[] = {
            40.f,  -1.f, 40.0f,  // top right
            40.f, -1.f, -40.0f,  // bottom right
            -40.f, -1.f, -40.0f,  // bottom left

            -40.f, -1.f, -40.0f,  // bottom left
            -40.f,  -1.f, 40.0f,   // top left
            40.f,  -1.f, 40.0f,  // top right
    };

    unsigned int floorPosVBO;
    glGenBuffers(1, &floorPosVBO);
    glBindBuffer(GL_ARRAY_BUFFER, floorPosVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void prepareTeapotMatrices(){
    auto teapotModel = glm::mat4(1.f);
    auto teapotView = glm::mat4(1.f);
    auto teapotProjection = glm::mat4(1.f);

    unsigned int modelLoc = glGetUniformLocation(teapotProgram.GetID(), "model");
    unsigned int viewLoc = glGetUniformLocation(teapotProgram.GetID(), "view");
    unsigned int projectionLoc = glGetUniformLocation(teapotProgram.GetID(), "projection");

    glm::vec3 cameraPos;
    cameraPos.x = cos(glm::radians(cameraxRot)) * cos(glm::radians(camerayRot));
    cameraPos.y = sin(glm::radians(camerayRot));
    cameraPos.z = sin(glm::radians(cameraxRot)) * cos(glm::radians(camerayRot));
    cameraPos = 50.f * cameraPos;

    teapotView = glm::lookAt(glm::vec3(cameraPos), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    // It should be about 5*bounding box size
    teapotProjection = glm::perspective(glm::radians(teapotZoom), 1.0f, 0.1f, 1500.f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &teapotModel[0][0]);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &teapotView[0][0]);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &teapotProjection[0][0]);

    unsigned int lightPosLoc = glGetUniformLocation(teapotProgram.GetID(), "lightPos");
    unsigned int viewPosLoc = glGetUniformLocation(teapotProgram.GetID(), "viewPos");
    glUniform3fv(lightPosLoc, 1, &lightPos[0]);
    glUniform3fv(viewPosLoc, 1, &cameraPos[0]);

    // The shadow matrix is T*S*shadProj*shadView*teapotModel.
    // where t is a translation of (0.5,0.5,0.5) and S is a scale of (0.5).
    auto shadowMatrix = glm::mat4(1.f);
    shadowMatrix = shadowProjection * shadowView * teapotModel;
    shadowMatrix = glm::scale(shadowMatrix, glm::vec3(0.5));
    shadowMatrix = glm::translate(shadowMatrix, glm::vec3(0.5, 0.5, 0.5));
    unsigned int shadowMatrixLoc = glGetUniformLocation(teapotProgram.GetID(), "shadowMatrix");
    glUniformMatrix4fv(shadowMatrixLoc, 1, GL_FALSE, &shadowMatrix[0][0]);

}

void prepareShadowMatrices(){
    unsigned int viewLoc = glGetUniformLocation(shadowProgram.GetID(), "view");
    unsigned int projectionLoc = glGetUniformLocation(shadowProgram.GetID(), "projection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &shadowView[0][0]);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &shadowProjection[0][0]);
}