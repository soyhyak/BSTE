#ifndef BSTE_H
#define BSTE_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QLabel>
#include <QString>
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QVector>
#include <QMenu>

class BSTE;

class SimpleHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    enum Language { Plain, Cpp, Python };
    SimpleHighlighter(QTextDocument *document = nullptr);
    void setLanguage(Language lang);
protected:
    void highlightBlock(const QString &text) override;
private:
    Language language = Plain;
    struct Rule { QRegularExpression pattern; QTextCharFormat format; };
    QVector<Rule> rules;
    void updateRules();
};

class LineNumberArea : public QWidget {
public:
    LineNumberArea(BSTE *editor);
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    BSTE *bsteEditor;
};

class BSTE : public QPlainTextEdit {
    Q_OBJECT
public:
    BSTE(QWidget *parent = nullptr);
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    void setLanguage(SimpleHighlighter::Language lang);
    void setTabSize(int size);
    int tabSize() const;
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void setWordWrap(bool wrap);
protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
private slots:
    void updateLineNumberAreaWidth(int);
    void updateLineNumberArea(const QRect &, int);
    void highlightCurrentLine();
    void updatePalette();
private:
    QWidget *lineNumberArea;
    SimpleHighlighter *highlighter = nullptr;
    int currentLineNumber = 0;
    int tabSizeValue = 4;
    int zoomLevel = 0;
    QFont baseFont;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    void closeEvent(QCloseEvent *event) override;
private slots:
    void newFile();
    void openFile();
    bool save();
    bool saveAs();
    void documentWasModified();
    void updateStatusBar();
    void showFindDialog();
    void findNext();
    void showReplaceDialog();
    void replaceText();
    void replaceAllText();
    void goToLine();
    void changeLanguage(SimpleHighlighter::Language lang);
    void changeTabSize();
    void toggleWordWrap();
    void zoomIn();
    void zoomOut();
    void zoomReset();
private:
    void initEditor();
    void initActions();
    void initMenus();
    void initStatusBar();
    void loadSettings();
    void saveSettings();
    bool maybeSave();
    void loadFile(const QString &fileName);
    bool writeFile(const QString &fileName);
    void setCurrentFile(const QString &fileName);
    BSTE *editor;
    QString curFile;
    QLabel *statusLabel;
    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *viewMenu;
    QMenu *languageMenu;
    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *exitAct;
    QAction *findAct;
    QAction *replaceAct;
    QAction *goToLineAct;
    QAction *tabSizeAct;
    QAction *wordWrapAct;
    QAction *plainAct;
    QAction *cppAct;
    QAction *pythonAct;
    QAction *zoomInAct;
    QAction *zoomOutAct;
    QAction *zoomResetAct;
    QWidget *searchWidget;
    QLineEdit *findInput;
    QLineEdit *replaceInput;
    QPushButton *nextBtn;
    QPushButton *replaceBtn;
    QPushButton *replaceAllBtn;
    QPushButton *closeBtn;
};

#endif