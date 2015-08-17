#include "MonospaceGrid.h"
#include <QPainter>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QWheelEvent>
#include <algorithm>

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
    
    std::vector<ArrowInfo> arrows;
    
    auto y = _charHeight;
    auto r = _curRow;
    for (; r <= _model->getMaxRow(); r += _model->getRowStep()) {
        x = 0;
        for (auto c = 0; c < (int)_columns.size(); ++c) {
            auto z = _model->getCell(r, c);
            auto isHighlighted = _model->isHighlighted(r, c);
            painter.setPen(QPen(QColor(isHighlighted ? Qt::red : Qt::black)));
            painter.drawText(x, y, z);
            x += getColumnWidth(c);
            
            ArrowInfo arrow;
            if (c == _arrowsColumn && _model->pointsTo(r, arrow.to, arrow.highlighted)) {
                arrow.from = r;
                arrows.push_back(arrow);
            }
        }
        y += _charHeight;
        if (y > event->rect().height())
            break;
    }
    _lastRow = r;
    drawArrows(arrows, painter);
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
    }
    update();
}
void MonospaceGrid::wheelEvent(QWheelEvent* event) {
    if (!_scrollable)
        return;
    auto d = event->delta() / 10.;
    auto newRow = (__int128)_curRow - (__int128)d;
    if (newRow > (__int128)~0ull || newRow < 0)
        return;
    _curRow -= d;
    _curRow -= _curRow % _model->getRowStep();
    update();
}

void MonospaceGrid::setScrollable(bool value) {
    _scrollable = value;
}

void MonospaceGrid::drawArrow(ArrowInfo arrow, int pos, QPainter& painter) {
    if (_arrowsColumn == -1)
        return;
    auto space = 8;
    auto right = -15;
    for (int c = 0; c <= _arrowsColumn; ++c) {
        right += getColumnWidth(c);
    }
    auto x = right - space * pos;
    auto y = [=](uint64_t row) {
        return (row - _curRow) / _model->getRowStep() * _charHeight + 2 * _charHeight / 3;
    };
    painter.setPen(QPen(QColor(arrow.highlighted ? Qt::red : Qt::black)));
    painter.drawLine(right, y(arrow.from), x, y(arrow.from));
    painter.drawLine(x, y(arrow.from), x, y(arrow.to));
    if (0u < y(arrow.to) && y(arrow.to) < (uint)height()) {
        painter.drawLine(x, y(arrow.to), right, y(arrow.to));
        painter.drawLine(right - 5, y(arrow.to) + 6, right, y(arrow.to) + 1);
        painter.drawLine(right - 5, y(arrow.to) - 5, right, y(arrow.to));
    }
}

bool MonospaceGridModel::pointsTo(uint64_t row, uint64_t& to, bool& highlighted) {
    return false;
}

void MonospaceGrid::setArrowsColumn(int col) {
    _arrowsColumn = col;
}

int MonospaceGrid::getColumnWidth(int index) {
    return _columns.at(index).charsWidth * _charWidth + 5;
}

void MonospaceGrid::drawArrows(std::vector<ArrowInfo> arrows, QPainter& painter) {
    if (arrows.empty())
        return;
    std::sort(begin(arrows), end(arrows), [](auto l, auto r) {
        return std::min(l.from, l.to) < std::min(r.from, r.to);
    });
    std::vector<std::vector<ArrowInfo>> groups;
    groups.push_back({});
    uint64_t min = std::max(arrows.front().from, arrows.front().to);
    for (auto a : arrows) {
         if (min < std::min(a.from, a.to)) {
             groups.push_back({});
             min = std::max(a.from, a.to);
         }
        groups.back().push_back(a);
    }
    for (auto g : groups) {
        std::sort(begin(g), end(g), [](auto l, auto r) {
            return std::max(l.from, l.to) - std::min(l.from, l.to) <
                   std::max(r.from, r.to) - std::min(r.from, r.to);
        });
        int pos = 1;
        for (auto arrow : g) {
            drawArrow(arrow, pos++, painter);
        }
    }
}
