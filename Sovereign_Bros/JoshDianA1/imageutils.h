#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H

#include <QImage>
#include <QColor>

inline QImage cropTransparent(const QImage &img) {
    QImage formattedImg = img.convertToFormat(QImage::Format_ARGB32);
    int top = formattedImg.height();
    int bottom = 0;
    int left = formattedImg.width();
    int right = 0;
    
    for (int y = 0; y < formattedImg.height(); ++y) {
        bool rowHasAlpha = false;
        for (int x = 0; x < formattedImg.width(); ++x) {
            if (qAlpha(formattedImg.pixel(x, y)) > 0) {
                if (x < left) left = x;
                if (x > right) right = x;
                rowHasAlpha = true;
            }
        }
        if (rowHasAlpha) {
            if (y < top) top = y;
            if (y > bottom) bottom = y;
        }
    }
    
    if (left <= right && top <= bottom) {
        return formattedImg.copy(left, top, right - left + 1, bottom - top + 1);
    }
    return formattedImg;
}

#endif // IMAGEUTILS_H
