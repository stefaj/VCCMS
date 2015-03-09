/** *********************************************************************** **/
/** *********************************************************************** **/
/**     Created by:     Ruan (Baggins) Luies 23511354                       **/
/**     email:          23511354@nwu.ac.za                                  **/
/**     Project name:   Virtual Concierge Creator And Management System     **/
/**     File Name:      ModelMesh.h                                         **/
/**     From Date:      2015/02/24                                          **/
/**     To Date:        2015/10/01                                          **/
/** **********************************************************************  **/
/** *********************************************************************** **/

#ifndef MODELMESH_H
#define MODELMESH_H
#include <QVector>

class QVector2D;
class QVector3D;
class QString;

class ModelMesh
{
public:
    ModelMesh(QString);
    ~ModelMesh();
    void Draw();
    QVector<QVector2D> textureCoordinates;
    QVector<QVector3D> vertices;
    QVector<QVector3D> normals;
    QVector<int> vertexIndices, uvIndices, normalIndices;
    bool LoadOBJ(QString);
};

#endif // MODELMESH_H
