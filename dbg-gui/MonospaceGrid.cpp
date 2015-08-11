#include "MonospaceGrid.h"
#include <QPainter>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <QPaintEvent>

void MonospaceGrid::setModel(MonospaceGridModel* model) {
    _model = model;
    _columns.resize(model->getColumnCount());
    connect(model, &MonospaceGridModel::navigated, this, &MonospaceGrid::go);
    connect(model, &MonospaceGridModel::updated, this, [=]{ update(); });
}

void MonospaceGrid::setColumnWidth(int col, int chars) {
    _columns.at(col).charsWidth = chars;
}

void MonospaceGrid::selectRow(uint64_t row) {
    
}

void MonospaceGrid::highlightRow(uint64_t row) {
    auto it = _rows.find(row);
    if (it == end(_rows)) {
        _rows[row] = RowInfo{false, true};
    } else {
        it->second.isHighlighted = true;
    }
}

void MonospaceGrid::clearHighlights() {
    
}

void MonospaceGrid::go(uint64_t row) {
    _curRow = row;
}

void MonospaceGrid::paintEvent(QPaintEvent* event) {
    if (!_model)
        return;
    
    QPainter painter(this);
    painter.setFont(QFont("monospace", 10));
    auto charWidth = painter.fontMetrics().width('q');
    auto charHeight = painter.fontMetrics().height();
    
    int x = 0;
    painter.setPen(QPen(QColor(Qt::gray)));
    for (auto i = 0u; i < _columns.size() - 1; ++i) {
        x += _columns.at(i).charsWidth * charWidth;
        painter.drawLine(x, 0, x, event->rect().height());
    }
    
    auto y = charHeight;
    for (auto r = _curRow; r < _model->getMaxRow(); r += _model->getRowStep()) {
        x = 0;
        for (auto c = 0u; c < _columns.size(); ++c) {
            auto z = _model->getCell(r, c);
            auto info = _rows.find(r);
            auto isHighlighted = info != end(_rows) && info->second.isHighlighted;
            painter.setPen(QPen(QColor(isHighlighted ? Qt::red : Qt::black)));
            painter.drawText(x, y, z);
            x += _columns.at(c).charsWidth * charWidth + 5;
        }
        y += charHeight + 3;
        if (y > event->rect().height())
            break;
    }
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

