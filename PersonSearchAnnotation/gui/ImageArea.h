#ifndef IMAGEAREA_H
#define IMAGEAREA_H

#include "common/PersonBBox.hpp"
#include <QVector>
#include <QGraphicsView>
#include <QFlags>

class ImageArea : public QGraphicsView
{
  Q_OBJECT

public:
  enum PermissionFlag
  {
    AllowSelection = 0x1,
    AllowAnnotation = 0x2,
    AllowMoving = 0x5,
    AllowResizing = 0x9,
    AllowRemoving = 0x11
  };
  Q_DECLARE_FLAGS(PermissionFlags, PermissionFlag)

  enum Mode
  {
    ModeSelection,
    ModeAnnotationByHeadFeat,
    ModeAnnotationByDragDrop
  };

public:
  explicit ImageArea(QWidget* parent = 0);

  void reset();

  PermissionFlags getPermissionFlags() const;
  void setPermissionFlags(PermissionFlags permissionFlags);

  void setMode(Mode mode);
  void setImage(const QImage& image, int imageId);
  void setPersonBBoxes(const QVector<PersonBBox>& personBBoxes);

  PersonBBox getSelectedPersonBBox() const;
  QVector<PersonBBox> getPersonBBoxes() const;
  QVector<bool> getRemovedMarks() const;

  void setPersonIdOfSelectedBBox(int personId);

  void toggleHard();

  void clearSelection();
  void clearSelectionAfterMouseReleased();

signals:
  void personBBoxSelected();

protected:
  void wheelEvent(QWheelEvent* event);
  void drawBackground(QPainter* painter, const QRectF& rect);
  void mousePressEvent(QMouseEvent* event);
  void mouseMoveEvent(QMouseEvent* event);
  void mouseReleaseEvent(QMouseEvent* event);
  void keyPressEvent(QKeyEvent* event);

private:
  PermissionFlags permissionFlags_;
  Mode mode_;

  enum State
  {
    StateIdleForSelection,
    StateSelected,
    StateResizing,
    StateIdleForAnnotation,
    StateHeadMarked,
    StateDragging
  };
  State state_;

  enum CornerType
  {
    CornerTypeNone,
    CornerTypeLeftTop,
    CornerTypeLeftBottom,
    CornerTypeRightTop,
    CornerTypeRightBottom
  };

  QPointF headPoint_;
  QPointF anchorPoint_;

  QImage image_;
  int imageId_;

  QVector<PersonBBox> personBBoxes_;
  QVector<bool> removedMarks_;

  bool needClearSelection_;

  QGraphicsLineItem* horizontalRuler_;
  QGraphicsLineItem* verticalRuler_;

private slots:
  void rectItemsSelectionChanged();

private:
  void updateBehaviors();

  void scaleView(qreal scaleFactor);

  void removeHeadMark();
  void removeRulers();

  CornerType atCorner(const QRectF& rect, const QPointF& point);

  QGraphicsRectItem* getPersonBBoxItem(int index);
  QGraphicsRectItem* getPersonIdRectItem(int index);
  QGraphicsSimpleTextItem* getPersonIdTextItem(int index);

  int addPersonBBox(qreal x, qreal y, qreal width, qreal height);
  void drawPersonBBox(const PersonBBox& personBBox, int index);
  void updatePersonIdPos(QGraphicsRectItem* bbox);
  void syncSelectedPersonBBox();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ImageArea::PermissionFlags)

#endif // IMAGEAREA_H
