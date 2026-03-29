#ifndef BSTE_H
#define BSTE_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QLabel>
#include <QString>
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>

class BSTE;

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
protected:
    void resizeEvent(QResizeEvent *event) override;
private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
private:
    QWidget *lineNumberArea;
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
    void findNext(const QString &text);
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
    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *exitAct;
    QAction *findAct;

    QWidget *findWidget;
    QLineEdit *findInput;
};

#endif