#include "bste.h"
#include <QPainter>
#include <QTextBlock>
#include <QAction>
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QSaveFile>
#include <QFileInfo>
#include <QCloseEvent>
#include <QSettings>
#include <QVBoxLayout>
#include <QHBoxLayout>

LineNumberArea::LineNumberArea(BSTE *editor) : QWidget(editor), bsteEditor(editor) {}
QSize LineNumberArea::sizeHint() const { return QSize(bsteEditor->lineNumberAreaWidth(), 0); }
void LineNumberArea::paintEvent(QPaintEvent *event) { bsteEditor->lineNumberAreaPaintEvent(event); }

BSTE::BSTE(QWidget *parent) : QPlainTextEdit(parent) {
    lineNumberArea = new LineNumberArea(this);
    connect(this, &BSTE::blockCountChanged, this, &BSTE::updateLineNumberAreaWidth);
    connect(this, &BSTE::updateRequest, this, &BSTE::updateLineNumberArea);
    updateLineNumberAreaWidth(0);
    setWordWrapMode(QTextOption::NoWrap);
}

int BSTE::lineNumberAreaWidth() {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) { max /= 10; digits++; }
    return 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void BSTE::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void BSTE::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy) lineNumberArea->scroll(0, dy);
    else lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    if (rect.contains(viewport()->rect())) updateLineNumberAreaWidth(0);
}

void BSTE::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void BSTE::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(240, 240, 240));
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor(120, 120, 120));
            painter.drawText(0, top, lineNumberArea->width() - 5, fontMetrics().height(), Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    initEditor();
    initActions();
    initMenus();
    initStatusBar();
    loadSettings();
    setCurrentFile(QString());
}

MainWindow::~MainWindow() {}

void MainWindow::initEditor() {
    QWidget *container = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    findWidget = new QWidget(this);
    QHBoxLayout *findLayout = new QHBoxLayout(findWidget);
    findInput = new QLineEdit(this);
    QPushButton *nextBtn = new QPushButton("Next", this);
    QPushButton *closeFindBtn = new QPushButton("X", this);
    closeFindBtn->setFixedWidth(30);

    findLayout->addWidget(new QLabel("Find:"));
    findLayout->addWidget(findInput);
    findLayout->addWidget(nextBtn);
    findLayout->addWidget(closeFindBtn);
    findWidget->setVisible(false);

    editor = new BSTE(this);
    layout->addWidget(findWidget);
    layout->addWidget(editor);
    setCentralWidget(container);

    QFont font("Monospace", 10);
    editor->setFont(font);

    connect(editor->document(), &QTextDocument::contentsChanged, this, &MainWindow::documentWasModified);
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateStatusBar);
    connect(nextBtn, &QPushButton::clicked, [this](){ findNext(findInput->text()); });
    connect(closeFindBtn, &QPushButton::clicked, [this](){ findWidget->setVisible(false); });
    connect(findInput, &QLineEdit::returnPressed, [this](){ findNext(findInput->text()); });
}

void MainWindow::initActions() {
    newAct = new QAction(tr("&New"), this);
    newAct->setShortcut(QKeySequence::New);
    connect(newAct, &QAction::triggered, this, &MainWindow::newFile);

    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, &QAction::triggered, this, &MainWindow::openFile);

    saveAct = new QAction(tr("&Save"), this);
    saveAct->setShortcut(QKeySequence::Save);
    connect(saveAct, &QAction::triggered, this, &MainWindow::save);

    saveAsAct = new QAction(tr("Save &As..."), this);
    saveAsAct->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::saveAs);

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(QKeySequence::Quit);
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    findAct = new QAction(tr("&Find"), this);
    findAct->setShortcut(QKeySequence::Find);
    connect(findAct, &QAction::triggered, this, &MainWindow::showFindDialog);
}

void MainWindow::initMenus() {
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(findAct);
}

void MainWindow::initStatusBar() {
    statusLabel = new QLabel(" Line: 1, Col: 1 ");
    statusBar()->addPermanentWidget(statusLabel);
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::updateStatusBar() {
    int line = editor->textCursor().blockNumber() + 1;
    int col = editor->textCursor().columnNumber() + 1;
    statusLabel->setText(tr(" Line: %1, Col: %2 ").arg(line).arg(col));
}

void MainWindow::showFindDialog() {
    findWidget->setVisible(true);
    findInput->setFocus();
    findInput->selectAll();
}

void MainWindow::findNext(const QString &text) {
    if (!editor->find(text)) {
        editor->moveCursor(QTextCursor::Start);
        editor->find(text);
    }
}

void MainWindow::loadSettings() {
    QSettings settings("BSTE-Project", "BSTE");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}

void MainWindow::saveSettings() {
    QSettings settings("BSTE-Project", "BSTE");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
}

void MainWindow::newFile() {
    if (maybeSave()) {
        editor->clear();
        setCurrentFile(QString());
    }
}

void MainWindow::openFile() {
    if (maybeSave()) {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("All Files (*)"));
        if (!fileName.isEmpty()) loadFile(fileName);
    }
}

bool MainWindow::save() {
    return curFile.isEmpty() ? saveAs() : writeFile(curFile);
}

bool MainWindow::saveAs() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), "", tr("All Files (*)"));
    return fileName.isEmpty() ? false : writeFile(fileName);
}

void MainWindow::documentWasModified() {
    setWindowModified(editor->document()->isModified());
}

void MainWindow::loadFile(const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not open file."));
        return;
    }
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    editor->setPlainText(in.readAll());
    setCurrentFile(fileName);
}

bool MainWindow::writeFile(const QString &fileName) {
    QSaveFile file(fileName);
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8);
        out << editor->toPlainText();
        if (!file.commit()) return false;
    } else return false;
    setCurrentFile(fileName);
    return true;
}

void MainWindow::setCurrentFile(const QString &fileName) {
    curFile = fileName;
    editor->document()->setModified(false);
    setWindowModified(false);
    setWindowTitle(tr("%1[*] - BSTE").arg(curFile.isEmpty() ? "untitled.txt" : QFileInfo(curFile).fileName()));
}

bool MainWindow::maybeSave() {
    if (!editor->document()->isModified()) return true;
    const auto ret = QMessageBox::warning(this, "BSTE", tr("Save changes?"), QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save) return save();
    return ret != QMessageBox::Cancel;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (maybeSave()) {
        saveSettings();
        event->accept();
    } else event->ignore();
}