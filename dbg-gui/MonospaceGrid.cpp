#include "MonospaceGrid.h"
#include <QPainter>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <QPaintEvent>

void MonospaceGrid::setModel(MonospaceGridModel* model) {
    _model = model;
    _columns.resize(model->getColumnCount());
    connect(model, &MonospaceGridModel::navigated, this, &MonospaceGrid::navigate);
    connect(model, &MonospaceGridModel::updated, this, [=]{ update(); });
}

void MonospaceGrid::setColumnWidth(int col, int chars) {
    _columns.at(col).charsWidth = chars;
}

void MonospaceGrid::paintEvent(QPaintEvent* event) {
    if (!_model)
        return;
    
    QPainter painter(this);
    painter.setFont(QFont("monospace", 10));
    _charWidth = painter.fontMetrics().width('q');
    _charHeight = painter.fontMetrics().height();
    
    int x = 0;
    painter.setPen(QPen(QColor(Qt::gray)));
    for (auto i = 0u; i < _columns.size() - 1; ++i) {
        x += _columns.at(i).charsWidth * _charWidth;
        painter.drawLine(x, 0, x, event->rect().height());
    }
    
    auto y = _charHeight;
    auto r = _curRow;
    for (; r <= _model->getMaxRow(); r += _model->getRowStep()) {
        x = 0;
        for (auto c = 0u; c < _columns.size(); ++c) {
            auto z = _model->getCell(r, c);
            auto isHighlighted = _model->isHighlighted(r, c);
            painter.setPen(QPen(QColor(isHighlighted ? Qt::red : Qt::black)));
            painter.drawText(x, y, z);
            x += _columns.at(c).charsWidth * _charWidth + 5;
        }
        y += _charHeight + 3;
        if (y > event->rect().height())
            break;
    }
    _lastRow = r;
}
MonospaceGridModel::~MonospaceGridModel() { }

void MonospaceGridModel::update() {
    emit updated();
}

void MonospaceGridModel::navigate(uint64_t row) {
    emit navigated(row);
}

uint64_t MonospaceGridModel::getRowStep() {
    return 1;
}

bool MonospaceGridModel::isHighlighted(uint64_t row, int col) {
    return false;
}

void MonospaceGrid::navigate(uint64_t row) {
    if (row < _curRow || row > _lastRow) {
        _curRow = row;
        update();
    }
}

