/* Copyright 2015 Ruan Luies */
#ifndef VIRTUAL_CREATOR_OBJECTS_NODEHANDLER_H_
#define VIRTUAL_CREATOR_OBJECTS_NODEHANDLER_H_

#include <QVector>
#include <QString>
#include <qdebug.h>
#include "Objects/Node.h"

class QVector3D;
class NodeHandler {
 public:
    NodeHandler();
    void AddNode(Node* node);
    Node NodeFromIndex(unsigned int index);
    void AddNodeLink(int index, QString* name);
    void AddNodeLinkbyIndex(int index1, int index2);
    void CalculateShortest(int start, int finish);
    void ReadFilePVC(QString filename);
    int count();
    int pathcount();
    int pathindex(int shortest_index);

 private:
    QVector<Node*> premises;
    QVector<int> shortest;
};

#endif  // VIRTUAL_CREATOR_OBJECTS_NODEHANDLER_H_
