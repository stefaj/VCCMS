/** *********************************************************************** **/
/** *********************************************************************** **/
/**     Created by:     Ruan (Baggins) Luies 23511354                       **/
/**     email:          23511354@nwu.ac.za                                  **/
/**     Project name:   Virtual Concierge Creator And Management System     **/
/**     File Name:      RenderState.cpp                                     **/
/**     From Date:      2015/02/24                                          **/
/**     To Date:        2015/10/01                                          **/
/** **********************************************************************  **/
/** *********************************************************************** **/

#include "VCCMStates/RenderState.h"
#include <qdebug.h>

RenderState::RenderState(): m_program(0), m_t(0)
{
    a = 0;
    m_rotationx = 0;
    // clear the textures
    textures.clear();

    m_position = new QVector3D(0,0,0);
    m_handler = new NodeHandler();

    m_handler->ReadFilePVC(":/Premises");
    m_handler->CalculateShortest(0,7);

    // texture test
    QOpenGLTexture *texture = new QOpenGLTexture(QImage(":/Texture0").mirrored());
    texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
    textures.append(texture);

}

void RenderState::paint()
{
    m_rotationx += 0.16f;
    a++;
    if(a>112)
        a =0;
    m_handler->CalculateShortest(0,a/16);
    // define a view matrix
    QMatrix4x4 vMatrix;
    // whenever content is not loaded, load the content
    if(!m_program){LoadContent();}
    // set the uniform value t for the shader (give it a new value)
    m_program->setUniformValue("t", (float) m_t);
    // setup the viewport for opengl
    glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());
    // initialize the projection matrix
    pMatrix.setToIdentity();
    // set the projection matrix
    pMatrix.perspective(60, (float) m_viewportSize.width() / (float) m_viewportSize.height(), 1.0f, 1000.0f);
    // enable the scene's depth mask
    glDepthMask(GL_TRUE);
    // clear the depth z = 0.0f -> 1.0f
    glClearDepth(1.0f);
    // enable the scene's depth test
    glEnable(GL_DEPTH_TEST);
    // enable cullmode CCW (counter clockwise)
    glEnable(GL_CULL_FACE);
    // clear the background color for rendering
    glClearColor(0.97, 0.97, 0.97, 1);
    // clear the color and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // setup camera
    QMatrix4x4 cameraTransformation;
    // rotation in the y - axis
    cameraTransformation.rotate(0, 0, 1, 0);
    // rotation in the x - axis
    cameraTransformation.rotate(-90+m_rotationx, 1, 0, 0);
    // transform the camera's position with respect to the rotation matrix
    QVector3D cameraPosition = cameraTransformation * QVector3D(0.0f, 0, 60.0f);
    // define the direction of the camera's up vector
    QVector3D cameraUpDirection = cameraTransformation * QVector3D(0, 1, 0);
    // implement and transform the camera
    vMatrix.lookAt(cameraPosition, QVector3D(0, 0, 0), cameraUpDirection);
    // draw each node to the scene
    for(int x = 0; x<m_handler->count();x++)
    {
      for(int l = 0;l<m_handler->NodeFromIndex(x).countConnected();l++)
        DrawLine(m_handler->NodeFromIndex(x).Position(),m_handler->NodeFromIndex(m_handler->NodeFromIndex(x).getConnectedIndex(l)).Position(),
               vMatrix,QMatrix4x4(),QMatrix4x4(),QVector3D(0,0,0));

      // this is for the model transformation
      QMatrix4x4 mMatrix;
      // transform the position locally
      mMatrix.translate(m_handler->NodeFromIndex(x).Position());
      /*mMatrix.scale(4.0);*/
      // draw different types of nodes, (connected & unconnected nodes & best path)
      DrawModel(node,vMatrix,mMatrix,QMatrix4x4()/*,0*/,m_handler->NodeFromIndex(x).getColor());
    }

    // disable the z buffer test
    glDisable(GL_DEPTH_TEST);
    for(int o = 0;o<m_handler->pathcount()-1;o++)
        DrawLine(m_handler->NodeFromIndex(m_handler->pathindex(o)).Position(),m_handler->NodeFromIndex(m_handler->pathindex(o+1)).Position(),
             vMatrix,QMatrix4x4(),QMatrix4x4(),QVector3D(0,1,0));

    glEnable(GL_DEPTH_TEST);
    // release the program for this frame
    m_program->release();
    // disable the cullmode for the frame
    glDisable(GL_CULL_FACE);
    // disable the depthtest for the frame
    glDisable(GL_DEPTH_TEST);
    // finish up the opengl frame
    glFinish();
}

void RenderState::LoadContent()
{
    // this initializes all the opengl functions
    initializeOpenGLFunctions();
    //load meshes
    box = new ModelMesh(":/block");
    node = new ModelMesh(":/Node");
    sky= new ModelMesh(":/Sky");
    wagen = new ModelMesh(":/Cube");
    // load shaders
    m_program = new QOpenGLShaderProgram();
    m_program->addShaderFromSourceFile(QOpenGLShader::Vertex,":/VertexShader");
    m_program->addShaderFromSourceFile(QOpenGLShader::Fragment,":/FragmentShader");
    m_program->link();
}

void RenderState::UpdateShaders(QMatrix4x4 wvp,QMatrix4x4 mvp, QMatrix4x4 rotate/*, GLuint texture*/,QVector3D color)
{
    // bind the current shader code
    m_program->bind();

    // bind the texture for the object
    textures.value(0)->bind();

    // update the colour of the object
    m_program->setUniformValue("col",color);

    // change the rotation of the object in the shader
    m_program->setUniformValue("rotationMatrix", rotate);

    // update model view projection
    m_program->setUniformValue("mvpMatrix", mvp * rotate);

    // update world view projection in the shader
    m_program->setUniformValue("wvpMatrix", pMatrix * wvp);

    // use GL_TEXTURE0
    m_program->setUniformValue("texture", 0);
}

void RenderState::ShaderDraw(ModelMesh *box)
{

    // convert the qstring to c-string for opengl purposes, this is the vertex variable in the shader files
    const char *vert ="vertex";//= vertex.toStdString().c_str();

    // convert the qstring to c-string for opengl purposes, this is the texture variable in the shader
    const char *textureCoordinate= "textureCoordinate";//= texCoord.toStdString().c_str();

    // convert the qstring to c-string for opengl, this is the normal variable in the shader code
    const char *normals = "normal";

    // load the vertices to the shaders
    m_program->setAttributeArray(vert, box->vertices.constData());

    // enable the shader attribute( vertices )
    m_program->enableAttributeArray(vert);

    // load the normals to the shaders
    m_program->setAttributeArray(normals, box->normals.constData());

    // enable the shader attribute( normals )
    m_program->enableAttributeArray(normals);

    // load the texture coordinates to the shaders
    m_program->setAttributeArray(textureCoordinate, box->textureCoordinates.constData());

    // enable the texture attribute
    m_program->enableAttributeArray(textureCoordinate);

    // draw the opengl vertices
    box->Draw();

    // disable the vertex attributes
    m_program->disableAttributeArray(vert);

    // disable the normal attributes
    m_program->disableAttributeArray(normals);

    // disable the Texture coordinates attributes
    m_program->disableAttributeArray(textureCoordinate);

    // release the current updated shader code (awaiting next frame)
    m_program->release();
   }

void RenderState::DrawLine(QVector3D point1, QVector3D point2,QMatrix4x4 wvp,QMatrix4x4 mvp, QMatrix4x4 rotate/*, GLuint texture*/,QVector3D color)
   {
     QVector< QVector3D > temp_vertices;
     temp_vertices.push_back(point1);
     temp_vertices.push_back(point2);
    UpdateShaders(wvp, mvp, rotate/*, texture*/, color);
    const char *vert ="vertex";//= vertex.toStdString().c_str();// convert the qstring to c-string for opengl purposes
    //const char *textureCoordinate= "textureCoordinate";//= texCoord.toStdString().c_str();// convert the qstring to c-string for opengl purposes
    const char *normals = "normal";// convert the qstring to c-string for opengl purposes
    m_program->setAttributeArray(vert, temp_vertices.constData());//load the vertices to the shaders
    m_program->enableAttributeArray(vert);//enable the shader attribute( vertices )
    m_program->setAttributeArray(normals, temp_vertices.constData());//load the normals to the shaders
    m_program->enableAttributeArray(normals);//enable the shader attribute( vertices )
    //m_program->setAttributeArray(textureCoordinate, box->textureCoordinates.constData());//load the texture coordinates to the shaders
   // m_program->enableAttributeArray(textureCoordinate);//enable the shader attribute( texture coordinates )
    glLineWidth(3.0);
    glDrawArrays(GL_LINES, 0, temp_vertices.size());

    m_program->disableAttributeArray(vert);// disable the vertex attributes
    m_program->disableAttributeArray(normals);// disable the normal attributes
    m_program->release(); // release the current updated shader code (awaiting next frame)
    temp_vertices.clear();
}

void RenderState::DrawModel(ModelMesh *box,QMatrix4x4 wvp,QMatrix4x4 mvp, QMatrix4x4 rotate/*, GLuint texture*/,QVector3D color)
 {
     UpdateShaders(wvp, mvp, rotate/*, texture*/, color);
     ShaderDraw(box);
 }

void RenderState::forwardmove()
{
    m_position->setX(m_position->x()+0.1f);
}

RenderState::~RenderState()
{
    delete m_program;
}
