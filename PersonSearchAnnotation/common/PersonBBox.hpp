#ifndef PERSONBBOX_H
#define PERSONBBOX_H

class PersonBBox
{
public:
  PersonBBox() : bboxId_(-1) {}
  ~PersonBBox() {}

  bool isNull() const { return bboxId_ == -1; }

  inline int getBBoxId() const { return bboxId_; }
  inline void setBBoxId(int bboxId) { bboxId_ = bboxId; }

  inline int getImageId() const { return imageId_; }
  inline void setImageId(int imageId) { imageId_ = imageId; }

  inline int getPersonId() const { return personId_; }
  inline void setPersonId(int personId) { personId_ = personId; }

  inline int x() const { return x_; }
  inline int y() const { return y_; }
  inline int width() const { return width_; }
  inline int height() const { return height_; }
  inline void setBBox(int x, int y, int width, int height) {
    x_ = x;
    y_ = y;
    width_ = width;
    height_ = height;
  }

  inline int isHard() const { return hard_; }
  inline void setHard(bool hard) { hard_ = hard; }

private:
  int bboxId_;
  int imageId_;
  int personId_;
  int x_;
  int y_;
  int width_;
  int height_;
  bool hard_;
};

#endif // PERSONBBOX_H
