#ifndef IMAGEFILE_HPP
#define IMAGEFILE_HPP

#include <QString>

class ImageFile
{
public:
  ImageFile(): imageId_(-1) {}
  ~ImageFile() {}

  bool isNull() const { return imageId_ == -1; }

  inline int getImageId() const { return imageId_; }
  void setImageId(int imageId) { imageId_ = imageId; }

  inline QString getPath() const { return path_; }
  void setPath(const QString& path) { path_ = path; }

  inline QString getAuthor() const { return author_; }
  void setAuthor(const QString& author) { author_ = author; }

private:
  int imageId_;
  QString path_;
  QString author_;
};

#endif // IMAGEFILE_HPP

