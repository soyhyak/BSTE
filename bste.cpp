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
#include <QKeyEvent>
#include <QInputDialog>
#include <QApplication>
#include <QEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QStringListModel>
#include <QAbstractItemView>

LineNumberArea::LineNumberArea(BSTE *editor) : QWidget(editor), bsteEditor(editor) {}
QSize LineNumberArea::sizeHint() const { return QSize(bsteEditor->lineNumberAreaWidth(), 0); }
void LineNumberArea::paintEvent(QPaintEvent *event) { bsteEditor->lineNumberAreaPaintEvent(event); }

SimpleHighlighter::SimpleHighlighter(QTextDocument *document) : QSyntaxHighlighter(document) { updateRules(); }
void SimpleHighlighter::setLanguage(Language lang) { language = lang; updateRules(); rehighlight(); }
void SimpleHighlighter::updateRules() {
    rules.clear();
    if (language == Plain) return;
    QTextCharFormat keywordFormat; keywordFormat.setForeground(Qt::darkBlue); keywordFormat.setFontWeight(QFont::Bold);
    QTextCharFormat typeFormat; typeFormat.setForeground(Qt::darkMagenta);
    QTextCharFormat stringFormat; stringFormat.setForeground(Qt::darkGreen);
    QTextCharFormat commentFormat; commentFormat.setForeground(Qt::darkGray); commentFormat.setFontItalic(true);
    QTextCharFormat numberFormat; numberFormat.setForeground(Qt::darkRed);
    if (language == Cpp) {
        static const QRegularExpression keywordRe("\\b(class|struct|enum|template|typename|using|namespace|public|private|protected|virtual|override|const|static|inline|if|else|for|while|do|switch|case|break|continue|return|new|delete|this|true|false)\\b");
        static const QRegularExpression typeRe("\\b(int|float|double|char|bool|void|size_t|uint8_t|uint32_t|std::string)\\b");
        rules.append({keywordRe, keywordFormat});
        rules.append({typeRe, typeFormat});
    } else if (language == Python) {
        static const QRegularExpression keywordRe("\\b(def|class|if|elif|else|for|while|return|import|from|as|try|except|finally|with|lambda|pass|break|continue|raise|assert|global|nonlocal|True|False|None)\\b");
        rules.append({keywordRe, keywordFormat});
    }
    static const QRegularExpression stringRe("\"[^\"]*\"|'[^']*'");
    static const QRegularExpression numberRe("\\b\\d+\\.?\\d*\\b");
    rules.append({stringRe, stringFormat});
    rules.append({numberRe, numberFormat});
    rules.append({QRegularExpression("//.*|/\\*.*\\*/"), commentFormat});
}
void SimpleHighlighter::highlightBlock(const QString &text) {
    for (const auto &rule : rules) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

BSTE::BSTE(QWidget *parent) : QPlainTextEdit(parent) {
    lineNumberArea = new LineNumberArea(this);
    highlighter = new SimpleHighlighter(document());
    completer = new QCompleter(this);
    completer->setWidget(this);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    connect(this, &BSTE::blockCountChanged, this, &BSTE::updateLineNumberAreaWidth);
    connect(this, &BSTE::updateRequest, this, &BSTE::updateLineNumberArea);
    connect(this, &BSTE::cursorPositionChanged, this, &BSTE::highlightCurrentLine);
    connect(this->document(), &QTextDocument::contentsChanged, this, &BSTE::updateCompleter);
    updateLineNumberAreaWidth(0);
    setWordWrapMode(QTextOption::NoWrap);
    baseFont = QFont("Monospace", 10);
    setFont(baseFont);
    setTabSize(4);
    updatePalette();
    enableAutoCompletion(false);
    setCenterOnScroll(true);
}
int BSTE::lineNumberAreaWidth() {
    int digits = 1; int max = qMax(1, blockCount());
    while (max >= 10) { max /= 10; digits++; }
    return 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}
void BSTE::updateLineNumberAreaWidth(int) { setViewportMargins(lineNumberAreaWidth(), 0, 0, 0); }
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
    painter.fillRect(event->rect(), palette().color(QPalette::Window));
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(currentLineNumber == blockNumber ? palette().color(QPalette::Highlight) : palette().color(QPalette::WindowText));
            painter.drawText(0, top, lineNumberArea->width() - 5, fontMetrics().height(), Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}
void BSTE::highlightCurrentLine() {
    currentLineNumber = textCursor().blockNumber();
    QList<QTextEdit::ExtraSelection> extraSelections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(palette().color(QPalette::AlternateBase));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    setExtraSelections(extraSelections);
    lineNumberArea->update();
}
void BSTE::updatePalette() {
    QPalette p = palette();
    setStyleSheet(QStringLiteral("QPlainTextEdit{background:%1;color:%2;}").arg(p.color(QPalette::Base).name(), p.color(QPalette::Text).name()));
    lineNumberArea->update();
}
void BSTE::changeEvent(QEvent *event) {
    if (event->type() == QEvent::ApplicationPaletteChange) updatePalette();
    QPlainTextEdit::changeEvent(event);
}
void BSTE::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) zoomIn();
        else zoomOut();
        event->accept();
    } else QPlainTextEdit::wheelEvent(event);
}
void BSTE::zoomIn() { zoomLevel = qMin(zoomLevel + 1, 20); setFont(QFont(baseFont.family(), baseFont.pointSize() + zoomLevel)); }
void BSTE::zoomOut() { zoomLevel = qMax(zoomLevel - 1, -10); setFont(QFont(baseFont.family(), baseFont.pointSize() + zoomLevel)); }
void BSTE::zoomReset() { zoomLevel = 0; setFont(baseFont); }
void BSTE::enableAutoCompletion(bool enable) {
    autoCompleteEnabled = enable;
    completer->setWidget(enable ? this : nullptr);
}
void BSTE::updateCompleter() {
    if (!autoCompleteEnabled) return;
    QString text = toPlainText();
    QStringList words = text.split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);
    words.removeDuplicates();
    if (words.size() > completerWordLimit) words = words.mid(0, completerWordLimit);
    completer->setModel(new QStringListModel(words, completer));
}
void BSTE::setLanguage(SimpleHighlighter::Language lang) { if (highlighter) highlighter->setLanguage(lang); }
void BSTE::setTabSize(int size) {
    tabSizeValue = qMax(1, size);
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * tabSizeValue);
}
int BSTE::tabSize() const { return tabSizeValue; }
void BSTE::setWordWrap(bool wrap) { setWordWrapMode(wrap ? QTextOption::WordWrap : QTextOption::NoWrap); }
void BSTE::keyPressEvent(QKeyEvent *event) {
    if (autoCompleteEnabled && completer && completer->popup()->isVisible() && (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return || event->key() == Qt::Key_Tab)) {
        completer->popup()->hide();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QPlainTextEdit::keyPressEvent(event);
        QTextCursor cursor = textCursor();
        QTextBlock block = cursor.block().previous();
        if (block.isValid()) {
            QString indent = block.text().left(block.text().indexOf(QRegularExpression("\\S")));
            cursor.insertText(indent);
            setTextCursor(cursor);
        }
        return;
    }
    QPlainTextEdit::keyPressEvent(event);
    if (autoCompleteEnabled && completer) {
        QTextCursor tc = textCursor();
        tc.select(QTextCursor::WordUnderCursor);
        QString prefix = tc.selectedText();
        if (prefix.length() >= 2) {
            completer->setCompletionPrefix(prefix);
            if (completer->completionCount() > 0) {
                QRect cr = cursorRect();
                cr.setWidth(completer->popup()->sizeHintForColumn(0) + completer->popup()->verticalScrollBar()->sizeHint().width());
                completer->complete(cr);
            }
        }
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
MainWindow::~MainWindow() {
    if (buildProcess) {
        buildProcess->kill();
        buildProcess->waitForFinished();
        delete buildProcess;
    }
}
void MainWindow::initEditor() {
    QWidget *container = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    searchWidget = new QWidget(this);
    QVBoxLayout *searchLayout = new QVBoxLayout(searchWidget);
    searchLayout->setContentsMargins(4, 2, 4, 2);
    searchLayout->setSpacing(2);
    QHBoxLayout *findLayout = new QHBoxLayout;
    findInput = new QLineEdit(this);
    nextBtn = new QPushButton("Next", this);
    closeBtn = new QPushButton("X", this); closeBtn->setFixedWidth(30);
    findLayout->addWidget(new QLabel("Find:"));
    findLayout->addWidget(findInput);
    findLayout->addWidget(nextBtn);
    findLayout->addWidget(closeBtn);
    searchLayout->addLayout(findLayout);
    QHBoxLayout *replaceLayout = new QHBoxLayout;
    replaceInput = new QLineEdit(this);
    replaceBtn = new QPushButton("Replace", this);
    replaceAllBtn = new QPushButton("All", this);
    replaceLayout->addWidget(new QLabel("Replace:"));
    replaceLayout->addWidget(replaceInput);
    replaceLayout->addWidget(replaceBtn);
    replaceLayout->addWidget(replaceAllBtn);
    searchLayout->addLayout(replaceLayout);
    searchWidget->setVisible(false);
    editor = new BSTE(this);
    consoleOutput = new QPlainTextEdit(this);
    consoleOutput->setReadOnly(true);
    consoleOutput->setMaximumHeight(220);
    consoleOutput->setVisible(false);
    clearConsoleBtn = new QPushButton("Clear", this);
    clearConsoleBtn->setFixedWidth(60);
    QWidget *consoleWidget = new QWidget;
    QHBoxLayout *consoleLayout = new QHBoxLayout(consoleWidget);
    consoleLayout->setContentsMargins(0, 0, 0, 0);
    consoleLayout->addWidget(consoleOutput);
    consoleLayout->addWidget(clearConsoleBtn);
    mainSplitter = new QSplitter(Qt::Vertical, container);
    mainSplitter->addWidget(searchWidget);
    mainSplitter->addWidget(editor);
    mainSplitter->addWidget(consoleWidget);
    mainSplitter->setStretchFactor(1, 1);
    layout->addWidget(mainSplitter);
    setCentralWidget(container);
    QFont font("Monospace", 10);
    editor->setFont(font);
    consoleOutput->setFont(font);
    connect(editor->document(), &QTextDocument::contentsChanged, this, &MainWindow::documentWasModified);
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateStatusBar);
    connect(nextBtn, &QPushButton::clicked, this, &MainWindow::findNext);
    connect(replaceBtn, &QPushButton::clicked, this, &MainWindow::replaceText);
    connect(replaceAllBtn, &QPushButton::clicked, this, &MainWindow::replaceAllText);
    connect(closeBtn, &QPushButton::clicked, this, [this](){ searchWidget->setVisible(false); });
    connect(findInput, &QLineEdit::returnPressed, this, &MainWindow::findNext);
    connect(replaceInput, &QLineEdit::returnPressed, this, &MainWindow::replaceText);
    connect(clearConsoleBtn, &QPushButton::clicked, this, &MainWindow::clearConsole);
}
void MainWindow::initActions() {
    newAct = new QAction(tr("&New"), this); newAct->setShortcut(QKeySequence::New); connect(newAct, &QAction::triggered, this, &MainWindow::newFile);
    openAct = new QAction(tr("&Open..."), this); openAct->setShortcut(QKeySequence::Open); connect(openAct, &QAction::triggered, this, &MainWindow::openFile);
    saveAct = new QAction(tr("&Save"), this); saveAct->setShortcut(QKeySequence::Save); connect(saveAct, &QAction::triggered, this, &MainWindow::save);
    saveAsAct = new QAction(tr("Save &As..."), this); saveAsAct->setShortcut(QKeySequence::SaveAs); connect(saveAsAct, &QAction::triggered, this, &MainWindow::saveAs);
    exitAct = new QAction(tr("E&xit"), this); exitAct->setShortcut(QKeySequence::Quit); connect(exitAct, &QAction::triggered, this, &QWidget::close);
    findAct = new QAction(tr("&Find"), this); findAct->setShortcut(QKeySequence::Find); connect(findAct, &QAction::triggered, this, &MainWindow::showFindDialog);
    replaceAct = new QAction(tr("&Replace"), this); replaceAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_H)); connect(replaceAct, &QAction::triggered, this, &MainWindow::showReplaceDialog);
    goToLineAct = new QAction(tr("Go to &Line"), this); goToLineAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G)); connect(goToLineAct, &QAction::triggered, this, &MainWindow::goToLine);
    tabSizeAct = new QAction(tr("Tab &Size..."), this); connect(tabSizeAct, &QAction::triggered, this, &MainWindow::changeTabSize);
    consoleAct = new QAction(tr("Show &Console"), this); consoleAct->setShortcut(QKeySequence("Ctrl+`")); consoleAct->setCheckable(true); connect(consoleAct, &QAction::triggered, this, &MainWindow::toggleConsole);
    autoCompleteAct = new QAction(tr("Auto-&Completion"), this); autoCompleteAct->setCheckable(true); connect(autoCompleteAct, &QAction::triggered, this, &MainWindow::toggleAutoCompletion);
    wordWrapAct = new QAction(tr("Word &Wrap"), this); wordWrapAct->setCheckable(true); connect(wordWrapAct, &QAction::triggered, this, &MainWindow::toggleWordWrap);
    plainAct = new QAction(tr("Plain"), this); connect(plainAct, &QAction::triggered, this, [this](){ changeLanguage(SimpleHighlighter::Plain); });
    cppAct = new QAction(tr("C++"), this); connect(cppAct, &QAction::triggered, this, [this](){ changeLanguage(SimpleHighlighter::Cpp); });
    pythonAct = new QAction(tr("Python"), this); connect(pythonAct, &QAction::triggered, this, [this](){ changeLanguage(SimpleHighlighter::Python); });
    zoomInAct = new QAction(tr("Zoom &In"), this); zoomInAct->setShortcut(QKeySequence::ZoomIn); connect(zoomInAct, &QAction::triggered, this, &MainWindow::zoomIn);
    zoomOutAct = new QAction(tr("Zoom &Out"), this); zoomOutAct->setShortcut(QKeySequence::ZoomOut); connect(zoomOutAct, &QAction::triggered, this, &MainWindow::zoomOut);
    zoomResetAct = new QAction(tr("Zoom &Reset"), this); zoomResetAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0)); connect(zoomResetAct, &QAction::triggered, this, &MainWindow::zoomReset);
    compileAct = new QAction(tr("&Compile"), this); compileAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B)); connect(compileAct, &QAction::triggered, this, &MainWindow::compileFile);
    runAct = new QAction(tr("&Run"), this); runAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R)); connect(runAct, &QAction::triggered, this, &MainWindow::runFile);
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
    editMenu->addAction(replaceAct);
    editMenu->addAction(goToLineAct);
    editMenu->addSeparator();
    editMenu->addAction(tabSizeAct);
    editMenu->addAction(autoCompleteAct);
    languageMenu = editMenu->addMenu(tr("&Language"));
    languageMenu->addAction(plainAct);
    languageMenu->addAction(cppAct);
    languageMenu->addAction(pythonAct);
    viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(consoleAct);
    viewMenu->addAction(wordWrapAct);
    viewMenu->addSeparator();
    viewMenu->addAction(zoomInAct);
    viewMenu->addAction(zoomOutAct);
    viewMenu->addAction(zoomResetAct);
    buildMenu = menuBar()->addMenu(tr("&Build"));
    buildMenu->addAction(compileAct);
    buildMenu->addAction(runAct);
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
    searchWidget->setVisible(true);
    findInput->setFocus();
    findInput->selectAll();
}
void MainWindow::showReplaceDialog() {
    searchWidget->setVisible(true);
    replaceInput->setFocus();
    replaceInput->selectAll();
}
void MainWindow::findNext() {
    if (!editor->find(findInput->text())) {
        editor->moveCursor(QTextCursor::Start);
        editor->find(findInput->text());
    }
}
void MainWindow::replaceText() {
    QTextCursor cursor = editor->textCursor();
    if (cursor.hasSelection() && cursor.selectedText() == findInput->text()) cursor.insertText(replaceInput->text());
    findNext();
}
void MainWindow::replaceAllText() {
    editor->moveCursor(QTextCursor::Start);
    int count = 0;
    while (editor->find(findInput->text())) {
        QTextCursor cursor = editor->textCursor();
        cursor.insertText(replaceInput->text());
        count++;
    }
    QMessageBox::information(this, "Replace All", tr("%1 occurrences replaced.").arg(count));
}
void MainWindow::goToLine() {
    bool ok;
    int line = QInputDialog::getInt(this, tr("Go to Line"), tr("Line number:"), editor->textCursor().blockNumber() + 1, 1, editor->document()->blockCount(), 1, &ok);
    if (ok) {
        QTextCursor cursor = editor->textCursor();
        cursor.setPosition(editor->document()->findBlockByNumber(line - 1).position());
        editor->setTextCursor(cursor);
    }
}
void MainWindow::changeLanguage(SimpleHighlighter::Language lang) { editor->setLanguage(lang); }
void MainWindow::changeTabSize() {
    bool ok;
    int size = QInputDialog::getInt(this, tr("Tab Size"), tr("Spaces per tab:"), editor->tabSize(), 1, 16, 1, &ok);
    if (ok) {
        editor->setTabSize(size);
        QSettings settings("BSTE-Project", "BSTE");
        settings.setValue("tabSize", size);
    }
}
void MainWindow::toggleConsole() {
    bool visible = consoleOutput->isVisible();
    consoleOutput->setVisible(!visible);
    consoleAct->setChecked(!visible);
}
void MainWindow::clearConsole() { consoleOutput->clear(); }
void MainWindow::toggleAutoCompletion() {
    bool enabled = autoCompleteAct->isChecked();
    editor->enableAutoCompletion(enabled);
    QSettings settings("BSTE-Project", "BSTE");
    settings.setValue("autoComplete", enabled);
}
void MainWindow::toggleWordWrap() {
    bool wrap = wordWrapAct->isChecked();
    editor->setWordWrap(wrap);
    QSettings settings("BSTE-Project", "BSTE");
    settings.setValue("wordWrap", wrap);
}
void MainWindow::zoomIn() { editor->zoomIn(); }
void MainWindow::zoomOut() { editor->zoomOut(); }
void MainWindow::zoomReset() { editor->zoomReset(); }
void MainWindow::compileFile() {
    if (curFile.isEmpty()) { QMessageBox::warning(this, "BSTE", tr("Save file first")); return; }
    QString ext = QFileInfo(curFile).suffix();
    if (buildProcess) buildProcess->kill();
    buildProcess = new QProcess(this);
    connect(buildProcess, &QProcess::readyReadStandardOutput, this, [this](){ appendToConsole(buildProcess->readAllStandardOutput()); });
    connect(buildProcess, &QProcess::readyReadStandardError, this, [this](){ appendToConsole(buildProcess->readAllStandardError()); });
    connect(buildProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::buildFinished);
    consoleOutput->appendPlainText("=== Compile started ===\n");
    if (ext == "cpp" || ext == "cxx" || ext == "cc") {
        QString out = curFile.left(curFile.lastIndexOf('.')) + ".out";
        buildProcess->start("g++", QStringList() << "-Wall" << "-O2" << curFile << "-o" << out);
    } else if (ext == "py") {
        buildProcess->start("python", QStringList() << "-m" << "py_compile" << curFile);
    } else {
        appendToConsole("Unsupported file type for compilation.\n");
        return;
    }
}
void MainWindow::runFile() {
    if (curFile.isEmpty()) { QMessageBox::warning(this, "BSTE", tr("Save file first")); return; }
    QString base = curFile.left(curFile.lastIndexOf('.'));
    if (QFileInfo(curFile).suffix() == "py") {
        buildProcess = new QProcess(this);
        connect(buildProcess, &QProcess::readyReadStandardOutput, this, [this](){ appendToConsole(buildProcess->readAllStandardOutput()); });
        connect(buildProcess, &QProcess::readyReadStandardError, this, [this](){ appendToConsole(buildProcess->readAllStandardError()); });
        consoleOutput->appendPlainText("=== Run started ===\n");
        buildProcess->start("python", QStringList() << curFile);
        connect(buildProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::buildFinished);
    } else {
        QString exe = base + ".out";
        if (!QFile::exists(exe)) { appendToConsole("Executable not found. Compile first.\n"); return; }
        buildProcess = new QProcess(this);
        connect(buildProcess, &QProcess::readyReadStandardOutput, this, [this](){ appendToConsole(buildProcess->readAllStandardOutput()); });
        connect(buildProcess, &QProcess::readyReadStandardError, this, [this](){ appendToConsole(buildProcess->readAllStandardError()); });
        consoleOutput->appendPlainText("=== Run started ===\n");
        buildProcess->start(exe, QStringList());
        connect(buildProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::buildFinished);
    }
}
void MainWindow::buildFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    appendToConsole(QString("\n=== Finished with code %1 ===\n").arg(exitCode));
    buildProcess->deleteLater();
    buildProcess = nullptr;
}
void MainWindow::appendToConsole(const QString &text) { consoleOutput->appendPlainText(text); }
void MainWindow::loadSettings() {
    QSettings settings("BSTE-Project", "BSTE");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    if (mainSplitter) mainSplitter->restoreState(settings.value("splitterState").toByteArray());
    int savedTab = settings.value("tabSize", 4).toInt();
    editor->setTabSize(savedTab);
    bool autoComp = settings.value("autoComplete", false).toBool();
    autoCompleteAct->setChecked(autoComp);
    editor->enableAutoCompletion(autoComp);
    bool wrap = settings.value("wordWrap", false).toBool();
    wordWrapAct->setChecked(wrap);
    editor->setWordWrap(wrap);
}
void MainWindow::saveSettings() {
    QSettings settings("BSTE-Project", "BSTE");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    if (mainSplitter) settings.setValue("splitterState", mainSplitter->saveState());
    settings.setValue("tabSize", editor->tabSize());
    settings.setValue("autoComplete", autoCompleteAct->isChecked());
    settings.setValue("wordWrap", wordWrapAct->isChecked());
}
void MainWindow::newFile() { if (maybeSave()) { editor->clear(); setCurrentFile(QString()); } }
void MainWindow::openFile() { if (maybeSave()) { QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("All Files (*)")); if (!fileName.isEmpty()) loadFile(fileName); } }
bool MainWindow::save() { return curFile.isEmpty() ? saveAs() : writeFile(curFile); }
bool MainWindow::saveAs() { QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), "", tr("All Files (*)")); return fileName.isEmpty() ? false : writeFile(fileName); }
void MainWindow::documentWasModified() { setWindowModified(editor->document()->isModified()); }
void MainWindow::loadFile(const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) { QMessageBox::critical(this, tr("Error"), tr("Could not open file.")); return; }
    QTextStream in(&file); in.setEncoding(QStringConverter::Utf8);
    editor->setPlainText(in.readAll());
    setCurrentFile(fileName);
}
bool MainWindow::writeFile(const QString &fileName) {
    QSaveFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) return false;
    QTextStream out(&file); out.setEncoding(QStringConverter::Utf8);
    out << editor->toPlainText();
    if (!file.commit()) return false;
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
    if (maybeSave()) { saveSettings(); event->accept(); } else event->ignore();
}