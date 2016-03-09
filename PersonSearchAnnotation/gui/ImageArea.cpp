#include "gui/ImageArea.h"
#include "utils/util_functions.h"
#include <cmath>
#include <algorithm>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QGraphicsItem>
#include <QDebug>

using namespace psa;

static const int BBoxIndex = 0;
static const int ItemType = 1;
enum GraphicsItemType
{
  PersonBBoxItem,
  PersonIdRectItem,
  PersonIdTextItem,
  RulerItem
};

static const QColor PresetColors[] = {
  QColor(255, 65, 54),
  QColor(61, 153, 112),
  QColor(0, 116, 217),
  QColor(133, 20, 75),
  QColor(0, 31, 63),
  QColor(240, 18, 190),
  QColor(1, 255, 112),
  QColor(127, 219, 255),
  QColor(255, 133, 27),
  QColor(176, 176, 176)
};
static const int NumPresetColors = 10;

static const int PersonIdRectWidth = 100;
static const int PersonIdRectHeight = 50;

ImageArea::ImageArea(QWidget* parent)
  : QGraphicsView(parent),
    permissionFlags_(AllowSelection),
    mode_(ModeSelection),
    state_(StateIdleForSelection),
    needClearSelection_(false),
    horizontalRuler_(NULL),
    verticalRuler_(NULL)
{
  QGraphicsScene* scene = new QGraphicsScene(this);
  scene->setItemIndexMethod(QGraphicsScene::BspTreeIndex);

  setScene(scene);
  setViewportUpdateMode(FullViewportUpdate);
  setRenderHint(QPainter::Antialiasing);
  setTransformationAnchor(AnchorUnderMouse);

  setMinimumSize(500, 500);

  connect(this->scene(), &QGraphicsScene::selectionChanged,
          this, &ImageArea::rectItemsSelectionChanged);
  updateBehaviors();
}

void ImageArea::reset()
{
  mode_ = ModeSelection;
  state_ = StateIdleForSelection;
  needClearSelection_ = false;
  image_ = QImage();
  imageId_ = -1;
  personBBoxes_.clear();
  removedMarks_.clear();
  updateBehaviors();
  scene()->clear();
  viewport()->update();
}

ImageArea::PermissionFlags ImageArea::getPermissionFlags() const
{
  return permissionFlags_;
}

void ImageArea::setPermissionFlags(PermissionFlags permissionFlags)
{
  permissionFlags_ = permissionFlags;
  updateBehaviors();
}

void ImageArea::setMode(Mode mode)
{
  if ((mode == ModeAnnotationByHeadFeat && !permissionFlags_.testFlag(AllowAnnotation)) ||
      (mode == ModeAnnotationByHeadFeat && !permissionFlags_.testFlag(AllowAnnotation)) ||
      (mode == ModeSelection && !permissionFlags_.testFlag(AllowSelection)) ||
      (mode == mode_)) {
    return;
  }
  if (mode_ == ModeSelection &&
      (mode == ModeAnnotationByHeadFeat || mode == ModeAnnotationByDragDrop)) {
    scene()->clearSelection();
    state_ = StateIdleForAnnotation;
  } else if (mode_ == ModeAnnotationByHeadFeat) {
    if (state_ == StateHeadMarked) removeHeadMark();
    if (mode == ModeSelection) {
      state_ = StateIdleForSelection;
    } else if (mode == ModeAnnotationByDragDrop) {
      state_ = StateIdleForAnnotation;
    }
  } else if (mode_ == ModeAnnotationByDragDrop) {
    removeRulers();
    if (mode == ModeSelection) {
      state_ = StateIdleForSelection;
    } else if (mode == ModeAnnotationByHeadFeat) {
      state_ = StateIdleForAnnotation;
    }
  }
  mode_ = mode;
  updateBehaviors();
}

void ImageArea::setImage(const QImage& image, int imageId)
{
  image_ = image;
  imageId_ = imageId;

  qreal w = static_cast<qreal>(image.width());
  qreal h = static_cast<qreal>(image.height());

  scene()->setSceneRect(-w / 2, -h / 2, w, h);
  scene()->clear();

  qreal bestScaleFactor = std::min((width() - 10) / w, (height() - 10) / h);
  resetTransform();
  scale(bestScaleFactor, bestScaleFactor);

  viewport()->update();
}

PersonBBox ImageArea::getSelectedPersonBBox() const
{
  if (scene()->selectedItems().size() != 1) {
    return PersonBBox();
  }

  int index = scene()->selectedItems().front()->data(BBoxIndex).toInt();
  return personBBoxes_[index];
}

QVector<PersonBBox> ImageArea::getPersonBBoxes() const
{
  return personBBoxes_;
}

QVector<bool> ImageArea::getRemovedMarks() const
{
  return removedMarks_;
}

void ImageArea::setPersonBBoxes(const QVector<PersonBBox>& personBBoxes)
{
  scene()->clear();
  for (int i = 0; i < personBBoxes.size(); ++i) {
    drawPersonBBox(personBBoxes[i], i);
  }
  personBBoxes_ = personBBoxes;
  removedMarks_.resize(personBBoxes_.size());
  for (int i = 0; i < removedMarks_.size(); ++i) {
    removedMarks_[i] = false;
  }
  if (permissionFlags_.testFlag(AllowAnnotation)) {
    horizontalRuler_ = scene()->addLine(
        -scene()->width() / 2, 0, scene()->width() / 2, 0,
        QPen(Qt::white, 5, Qt::SolidLine));
    verticalRuler_ = scene()->addLine(
        0, -scene()->height() / 2, 0, scene()->height() / 2,
        QPen(Qt::white, 5, Qt::SolidLine));
    horizontalRuler_->setData(ItemType, RulerItem);
    verticalRuler_->setData(ItemType, RulerItem);
    horizontalRuler_->setVisible(false);
    verticalRuler_->setVisible(false);
  }
  updateBehaviors();
}

void ImageArea::setPersonIdOfSelectedBBox(int personId)
{
  for (int i = 0; i < scene()->selectedItems().size(); ++i) {
    QGraphicsRectItem* bbox = dynamic_cast<QGraphicsRectItem*>(
        scene()->selectedItems().at(i));
    int index = bbox->data(BBoxIndex).toInt();
    // Update data
    PersonBBox& personBBox = personBBoxes_[index];
    personBBox.setPersonId(personId);
    // Update drawing
    QColor color = PresetColors[personId % NumPresetColors];
    bbox->setPen(QPen(color, 8, Qt::SolidLine));
    getPersonIdRectItem(index)->setPen(QPen(color, 0, Qt::SolidLine));
    getPersonIdRectItem(index)->setBrush(QBrush(color));
    getPersonIdTextItem(index)->setText(QString::number(personId));
  }
}

void ImageArea::toggleHard()
{
  foreach (QGraphicsItem* item, scene()->selectedItems()) {
    int index = item->data(BBoxIndex).toInt();
    PersonBBox& personBBox = personBBoxes_[index];
    personBBox.setHard(!personBBox.isHard());
    if (personBBox.isHard()) {
      getPersonIdTextItem(index)->setText(
          QString::number(personBBox.getPersonId()) + "*");
    } else {
      getPersonIdTextItem(index)->setText(
          QString::number(personBBox.getPersonId()));
    }
  }
}

void ImageArea::clearSelection()
{
  scene()->clearSelection();
  state_ = StateIdleForSelection;
}

void ImageArea::clearSelectionAfterMouseReleased()
{
  needClearSelection_ = true;
}

void ImageArea::wheelEvent(QWheelEvent* event)
{
  scaleView(pow(2.0, event->delta() / 240.0));
}

void ImageArea::drawBackground(QPainter* painter, const QRectF& /* rect */)
{
  QRectF sceneRect = this->sceneRect();
  if (!image_.isNull()) {
    painter->drawImage(sceneRect, image_);
  }
}

void ImageArea::mousePressEvent(QMouseEvent* event)
{
  if (mode_ == ModeSelection) {
    switch(state_) {
      case StateIdleForSelection:
      {
        // Pressed on the person id will have the highest priority to select.
        QGraphicsItem* item = itemAt(event->pos());
        if (item && item->data(ItemType).toInt() != PersonBBoxItem) {
          scene()->clearSelection();
          getPersonBBoxItem(item->data(BBoxIndex).toInt())->setSelected(true);
        } else {
          QGraphicsView::mousePressEvent(event);
        }
        break;
      }
      case StateSelected:
      {
        QGraphicsRectItem* bbox = dynamic_cast<QGraphicsRectItem*>(
            scene()->selectedItems().front());
        QPointF mousePos = mapToScene(event->pos());
        CornerType corner = atCorner(
            bbox->mapRectToScene(bbox->rect()), mousePos);

        if (corner == CornerTypeNone ||
            !permissionFlags_.testFlag(AllowResizing)) {
          setCursor(Qt::ArrowCursor);
          // Pressed on the person id will have the highest priority to select.
          QGraphicsItem* item = itemAt(event->pos());
          if (item && item->data(ItemType).toInt() != PersonBBoxItem) {
            scene()->clearSelection();
            getPersonBBoxItem(item->data(BBoxIndex).toInt())->setSelected(true);
          } else {
            QGraphicsView::mousePressEvent(event);
          }
          break;
        }

        switch(corner) {
          case CornerTypeLeftTop:
            setCursor(Qt::SizeFDiagCursor);
            anchorPoint_ = bbox->mapRectToScene(bbox->rect()).bottomRight();
            break;
          case CornerTypeLeftBottom:
            setCursor(Qt::SizeBDiagCursor);
            anchorPoint_ = bbox->mapRectToScene(bbox->rect()).topRight();
            break;
          case CornerTypeRightTop:
            setCursor(Qt::SizeBDiagCursor);
            anchorPoint_ = bbox->mapRectToScene(bbox->rect()).bottomLeft();
            break;
          case CornerTypeRightBottom:
            setCursor(Qt::SizeFDiagCursor);
            anchorPoint_ = bbox->mapRectToScene(bbox->rect()).topLeft();
            break;
          default:
            break;
        }
        state_ = StateResizing;
        break;
      }
      default:
        qDebug() << "mouse press  mode: " << mode_ << " state: " << state_;
    }
  } else if (mode_ == ModeAnnotationByHeadFeat) {
    switch(state_) {
      case StateIdleForAnnotation:
      {
        // Mark the head position
        headPoint_ = mapToScene(event->pos());
        QGraphicsRectItem* headDot = scene()->addRect(-3, -3, 6, 6,
            QPen(PresetColors[0], 0, Qt::SolidLine), QBrush(PresetColors[0]));
        headDot->setPos(headPoint_);
        state_ = StateHeadMarked;
        break;
      }
      case StateHeadMarked:
      {
        removeHeadMark();
        // Create a bbox with fixed ratio
        QPointF feetPoint = mapToScene(event->pos());
        QPointF centerPoint = (headPoint_ + feetPoint) / 2.0;
        qreal height = feetPoint.y() - headPoint_.y();
        if (height <= 0) {
          state_ = StateIdleForAnnotation;
          break;
        }
        qreal width = height * 0.375;
        qreal x = centerPoint.x() - width / 2.0 + scene()->width() / 2.0;
        qreal y = centerPoint.y() - height / 2.0 + scene()->height() / 2.0;
        int index = addPersonBBox(x, y, width, height);
        drawPersonBBox(personBBoxes_[index], index);
        state_ = StateIdleForAnnotation;
        break;
      }
      default:
        qDebug() << "mouse press  mode: " << mode_ << " state: " << state_;
    }
  } else if (mode_ == ModeAnnotationByDragDrop) {
    switch(state_) {
      case StateIdleForAnnotation:
      {
        anchorPoint_ = mapToScene(event->pos());
        // Just draw a new invisible bbox for convenience.
        QGraphicsRectItem* bbox = scene()->addRect(
            -0.1, -0.1, 0.2, 0.2,
            QPen(PresetColors[0], 8, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        bbox->setPos(anchorPoint_);
        bbox->setVisible(false);
        state_ = StateDragging;
        break;
      }
      default:
        qDebug() << "mouse press  mode: " << mode_ << " state: " << state_;
    }
  }
}

void ImageArea::mouseMoveEvent(QMouseEvent* event)
{
  if (mode_ == ModeSelection && permissionFlags_.testFlag(AllowResizing)) {
    switch(state_) {
      case StateSelected:
      {
        QGraphicsRectItem* bbox = dynamic_cast<QGraphicsRectItem*>(
            scene()->selectedItems().front());
        QPointF mousePos = mapToScene(event->pos());
        CornerType corner = atCorner(
            bbox->mapRectToScene(bbox->rect()), mousePos);
        switch(corner) {
          case CornerTypeLeftTop:
          case CornerTypeRightBottom:
            setCursor(Qt::SizeFDiagCursor);
            break;
          case CornerTypeLeftBottom:
          case CornerTypeRightTop:
            setCursor(Qt::SizeBDiagCursor);
            break;
          default:
          {
            setCursor(Qt::ArrowCursor);
            QGraphicsView::mouseMoveEvent(event);
            for (int i = 0; i < scene()->selectedItems().size(); ++i) {
              QGraphicsRectItem* bbox = dynamic_cast<QGraphicsRectItem*>(
                  scene()->selectedItems().at(i));
              updatePersonIdPos(bbox);
            }
          }
        }
        break;
      }
      case StateResizing:
      {
        QGraphicsRectItem* bbox = dynamic_cast<QGraphicsRectItem*>(
            scene()->selectedItems().front());
        QPointF movingPoint = mapToScene(event->pos());
        bbox->setPos((movingPoint + anchorPoint_) / 2.0);
        qreal w = std::abs(movingPoint.x() - anchorPoint_.x());
        qreal h = std::abs(movingPoint.y() - anchorPoint_.y());
        bbox->setRect(-w / 2.0, -h / 2.0, w, h);
        updatePersonIdPos(bbox);
        break;
      }
      default:
        QGraphicsView::mouseMoveEvent(event);
    }
  } else if (mode_ == ModeAnnotationByDragDrop) {
    QPointF movingPoint = mapToScene(event->pos());
    // Move the rulers
    if (!horizontalRuler_->isVisible()) horizontalRuler_->setVisible(true);
    if (!verticalRuler_->isVisible()) verticalRuler_->setVisible(true);
    horizontalRuler_->setLine(-scene()->width() / 2.0, movingPoint.y(),
                              scene()->width() / 2.0, movingPoint.y());
    verticalRuler_->setLine(movingPoint.x(), -scene()->height() / 2.0,
                            movingPoint.x(), scene()->height() / 2.0);
    switch(state_) {
      case StateDragging:
      {
        // Stretch the bbox
        QGraphicsRectItem* bbox = dynamic_cast<QGraphicsRectItem*>(
            scene()->items().front());
        if (!bbox->isVisible()) bbox->setVisible(true);
        bbox->setPos((movingPoint + anchorPoint_) / 2.0);
        qreal width = std::abs(movingPoint.x() - anchorPoint_.x());
        qreal height = std::abs(movingPoint.y() - anchorPoint_.y());
        bbox->setRect(-width / 2.0, -height / 2.0, width, height);
        break;
      }
      default:
        QGraphicsView::mouseMoveEvent(event);
    }
  } else {
    QGraphicsView::mouseMoveEvent(event);
  }
}

void ImageArea::mouseReleaseEvent(QMouseEvent* event)
{
  if (state_ == StateResizing) {
    state_ = StateSelected;
  } else if (state_ == StateDragging) {
    // Delete the temp bbox
    QGraphicsRectItem* bbox = dynamic_cast<QGraphicsRectItem*>(
        scene()->items().front());
    scene()->removeItem(bbox);
    delete bbox;
    // Draw a final bbox
    QPointF endPoint = mapToScene(event->pos());
    qreal x = std::min(anchorPoint_.x(), endPoint.x()) + scene()->width() / 2.0;
    qreal y = std::min(anchorPoint_.y(), endPoint.y()) + scene()->height() / 2.0;
    qreal width = std::abs(anchorPoint_.x() - endPoint.x());
    qreal height = std::abs(anchorPoint_.y() - endPoint.y());
    if (width == 0 || height == 0) {
      state_ = StateIdleForAnnotation;
      return;
    }
    int index = addPersonBBox(x, y, width, height);
    drawPersonBBox(personBBoxes_[index], index);
    state_ = StateIdleForAnnotation;
    return;
  } else {
    QGraphicsView::mouseReleaseEvent(event);
  }
  syncSelectedPersonBBox();
  if (needClearSelection_) {
    clearSelection();
    needClearSelection_ = false;
  }
}

void ImageArea::keyPressEvent(QKeyEvent* event)
{
  QPointF delta = mapToScene(1.0, 1.0) - mapToScene(0.0, 0.0);
  switch(event->key()) {
    case Qt::Key_Escape:
    {
      setCursor(Qt::ArrowCursor);
      if (state_ == StateSelected) {
        scene()->clearSelection();
        state_ = StateIdleForSelection;
      } else if (state_ == StateHeadMarked) {
        removeHeadMark();
        state_ = StateIdleForAnnotation;
      } else if (state_ == StateDragging) {
        removeRulers();
        QGraphicsRectItem* bbox = dynamic_cast<QGraphicsRectItem*>(
            scene()->items().front());
        scene()->removeItem(bbox);
        delete bbox;
        state_ = StateIdleForAnnotation;
      }
      break;
    }
    case Qt::Key_Left:
      if (permissionFlags_.testFlag(AllowMoving)) {
        for (int i = 0; i < scene()->selectedItems().size(); ++i) {
          QGraphicsRectItem* bbox = dynamic_cast<QGraphicsRectItem*>(
              scene()->selectedItems().at(i));
          bbox->setPos(bbox->pos().x() - delta.x(), bbox->pos().y());
          updatePersonIdPos(bbox);
        }
      }
      syncSelectedPersonBBox();
      break;
    case Qt::Key_Right:
      if (permissionFlags_.testFlag(AllowMoving)) {
        for (int i = 0; i < scene()->selectedItems().size(); ++i) {
          QGraphicsRectItem* bbox = dynamic_cast<QGraphicsRectItem*>(
              scene()->selectedItems().at(i));
          bbox->setPos(bbox->pos().x() + delta.x(), bbox->pos().y());
          updatePersonIdPos(bbox);
        }
      }
      syncSelectedPersonBBox();
      break;
    case Qt::Key_Up:
      if (permissionFlags_.testFlag(AllowMoving)) {
        for (int i = 0; i < scene()->selectedItems().size(); ++i) {
          QGraphicsRectItem* bbox = dynamic_cast<QGraphicsRectItem*>(
              scene()->selectedItems().at(i));
          bbox->setPos(bbox->pos().x(), bbox->pos().y() - delta.y());
          updatePersonIdPos(bbox);
        }
      }
      syncSelectedPersonBBox();
      break;
    case Qt::Key_Down:
      if (permissionFlags_.testFlag(AllowMoving)) {
        for (int i = 0; i < scene()->selectedItems().size(); ++i) {
          QGraphicsRectItem* bbox = dynamic_cast<QGraphicsRectItem*>(
              scene()->selectedItems().at(i));
          bbox->setPos(bbox->pos().x(), bbox->pos().y() + delta.y());
          updatePersonIdPos(bbox);
        }
      }
      syncSelectedPersonBBox();
      break;
    case Qt::Key_Delete:
    case Qt::Key_Backspace:
      if (permissionFlags_.testFlag(AllowRemoving)) {
        foreach (QGraphicsItem* bbox, scene()->selectedItems()) {
          int index = bbox->data(BBoxIndex).toInt();
          removedMarks_[index] = true;
          QGraphicsItem* personIdRect = getPersonIdRectItem(index);
          QGraphicsItem* personIdText = getPersonIdTextItem(index);
          scene()->removeItem(bbox);
          scene()->removeItem(personIdRect);
          scene()->removeItem(personIdText);
          delete bbox;
          delete personIdRect;
          delete personIdText;
        }
      }
      break;
    default:
      QGraphicsView::keyPressEvent(event);
      break;
  }
}

void ImageArea::rectItemsSelectionChanged()
{
  if (scene()->selectedItems().size() == 1) {
    state_ = StateSelected;
    emit personBBoxSelected();
  } else if (scene()->selectedItems().size() > 1) {
    for (int i = 1; i < scene()->selectedItems().size(); ++i) {
      scene()->selectedItems().at(i)->setSelected(false);
    }
  } else if (scene()->selectedItems().empty()) {
    state_ = StateIdleForSelection;
  }
}

void ImageArea::updateBehaviors()
{
  if (mode_ == ModeSelection && permissionFlags_.testFlag(AllowSelection)) {
    setDragMode(RubberBandDrag);
    QGraphicsItem::GraphicsItemFlags itemFlags = QGraphicsItem::ItemIsSelectable;
    if (permissionFlags_.testFlag(AllowMoving)) {
      itemFlags |= QGraphicsItem::ItemIsMovable;
    }
    foreach (QGraphicsItem* item, scene()->items()) {
      if (item->data(ItemType).toInt() == PersonBBoxItem) {
        item->setFlags(itemFlags);
      } else {
        item->setFlags(0);
      }
    }
  } else if (mode_ == ModeAnnotationByHeadFeat ||
             mode_ == ModeAnnotationByDragDrop) {
    setDragMode(NoDrag);
  }
}

void ImageArea::scaleView(qreal scaleFactor)
{
  qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(
        QRectF(0, 0, 1, 1)).width();
  if (factor < 0.07 || factor > 20) return;
  scale(scaleFactor, scaleFactor);
}

void ImageArea::removeHeadMark()
{
  scene()->removeItem(scene()->itemAt(headPoint_.x(), headPoint_.y(),
                                      transform()));
}

void ImageArea::removeRulers()
{
  horizontalRuler_->setVisible(false);
  verticalRuler_->setVisible(false);
}

ImageArea::CornerType ImageArea::atCorner(const QRectF& rect,
                                          const QPointF& point)
{
  qreal pad = 20;
  if (euclideanDist(rect.topLeft(), point) <= pad) return CornerTypeLeftTop;
  if (euclideanDist(rect.topRight(), point) <= pad) return CornerTypeRightTop;
  if (euclideanDist(rect.bottomLeft(), point) <= pad) return CornerTypeLeftBottom;
  if (euclideanDist(rect.bottomRight(), point) <= pad) return CornerTypeRightBottom;
  return CornerTypeNone;
}

QGraphicsRectItem* ImageArea::getPersonBBoxItem(int index)
{
  foreach (QGraphicsItem* item, scene()->items()) {
    if (item->data(BBoxIndex).toInt() == index &&
        item->data(ItemType).toInt() == PersonBBoxItem) {
      return dynamic_cast<QGraphicsRectItem*>(item);
    }
  }
  return NULL;
}

QGraphicsRectItem* ImageArea::getPersonIdRectItem(int index)
{
  foreach (QGraphicsItem* item, scene()->items()) {
    if (item->data(BBoxIndex).toInt() == index &&
        item->data(ItemType).toInt() == PersonIdRectItem) {
      return dynamic_cast<QGraphicsRectItem*>(item);
    }
  }
  return NULL;
}

QGraphicsSimpleTextItem* ImageArea::getPersonIdTextItem(int index)
{
  foreach (QGraphicsItem* item, scene()->items()) {
    if (item->data(BBoxIndex).toInt() == index &&
        item->data(ItemType).toInt() == PersonIdTextItem) {
      return dynamic_cast<QGraphicsSimpleTextItem*>(item);
    }
  }
  return NULL;
}

int ImageArea::addPersonBBox(qreal x, qreal y, qreal width, qreal height)
{
  PersonBBox personBBox;
  personBBox.setBBoxId(0);
  personBBox.setImageId(imageId_);
  personBBox.setPersonId(0);
  personBBox.setBBox(x, y, width, height);
  personBBox.setHard(false);
  personBBoxes_.push_back(personBBox);
  removedMarks_.push_back(false);
  return personBBoxes_.size() - 1;
}

void ImageArea::drawPersonBBox(const PersonBBox& personBBox, int index)
{
  qreal w = scene()->width();
  qreal h = scene()->height();
  QColor color = PresetColors[personBBox.getPersonId() % NumPresetColors];
  // bbox
  QGraphicsRectItem* bbox = scene()->addRect(
      -personBBox.width() / 2.0, -personBBox.height() / 2.0,
      personBBox.width(), personBBox.height(),
      QPen(color, 8, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
  bbox->setPos(personBBox.x() + personBBox.width() / 2.0 - w / 2.0,
               personBBox.y() + personBBox.height() / 2.0 - h / 2.0);
  bbox->setData(BBoxIndex, index);
  bbox->setData(ItemType, PersonBBoxItem);
  QRectF rect = bbox->mapRectToScene(bbox->boundingRect());
  // person id background rect
  QGraphicsRectItem* personIdRect = scene()->addRect(
      -PersonIdRectWidth / 2.0, -PersonIdRectHeight / 2.0,
      PersonIdRectWidth, PersonIdRectHeight,
      QPen(color, 0, Qt::SolidLine), QBrush(color));
  personIdRect->setPos(rect.x() + PersonIdRectWidth / 2.0,
                       rect.y() - PersonIdRectHeight / 2.0);
  personIdRect->setData(BBoxIndex, index);
  personIdRect->setData(ItemType, PersonIdRectItem);
  // person id text
  QString text = QString::number(personBBox.getPersonId());
  if (personBBox.isHard()) text += "*";
  QGraphicsSimpleTextItem* personIdText = scene()->addSimpleText(
      text, QFont("Arial", 42, QFont::Bold));
  personIdText->setPos(rect.x() + 10, rect.y() - PersonIdRectHeight - 3);
  personIdText->setBrush(QBrush(Qt::white));
  personIdText->setData(BBoxIndex, index);
  personIdText->setData(ItemType, PersonIdTextItem);
}

void ImageArea::updatePersonIdPos(QGraphicsRectItem* bbox)
{
  QRectF rect = bbox->mapRectToScene(bbox->boundingRect());
  int index = bbox->data(BBoxIndex).toInt();
  getPersonIdRectItem(index)->setPos(rect.x() + PersonIdRectWidth / 2.0,
                                     rect.y() - PersonIdRectHeight / 2.0);
  getPersonIdTextItem(index)->setPos(rect.x() + 10,
                                     rect.y() - PersonIdRectHeight - 3);
}

void ImageArea::syncSelectedPersonBBox()
{
  qreal w = scene()->width();
  qreal h = scene()->height();
  for (int i = 0; i < scene()->selectedItems().size(); ++i) {
    QGraphicsRectItem* item = dynamic_cast<QGraphicsRectItem*>(
        scene()->selectedItems().at(i));
    PersonBBox& personBBox = personBBoxes_[item->data(BBoxIndex).toInt()];
    QRectF rect = item->mapRectToScene(item->rect());
    personBBox.setBBox(rect.x() + w / 2.0, rect.y() + h / 2.0,
                       rect.width(), rect.height());
  }
}
