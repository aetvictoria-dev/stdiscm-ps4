#ifndef OCR_CLIENT_H
#define OCR_CLIENT_H

#include <QMainWindow>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QGridLayout>
#include <QFileDialog>
#include <QThread>
#include <QMutex>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <atomic>
#include <vector>
#include "ocr_service.grpc.pb.h"

class ImageResult : public QWidget {
    Q_OBJECT
public:
    explicit ImageResult(const QString& imagePath, QWidget* parent = nullptr);
    void setResult(const QString& text);
    void setError(const QString& error);

private:
    QLabel* imageLabel_;
    QLabel* resultLabel_;
    QString imagePath_;
};

class OCRWorker : public QThread {
    Q_OBJECT
public:
    OCRWorker(const QString& serverAddress,
              const QStringList& imagePaths,
              QObject* parent = nullptr);
    ~OCRWorker();
    void run() override;
    void stop();

signals:
    void resultReady(int index, const QString& text);
    void errorOccurred(int index, const QString& error);
    void progressUpdated(int current, int total);

private:
    QString serverAddress_;
    QStringList imagePaths_;
    std::atomic<bool> stopped_;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<ocrservice::OCRService::Stub> stub_;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onUploadClicked();
    void onResultReady(int index, const QString& text);
    void onErrorOccurred(int index, const QString& error);
    void onProgressUpdated(int current, int total);

private:
    void clearResults();
    void setupUI();

    QPushButton* uploadButton_;
    QProgressBar* progressBar_;
    QScrollArea* scrollArea_;
    QWidget* resultsContainer_;
    QGridLayout* resultsLayout_;
    QLineEdit* serverAddressEdit_;

    std::vector<ImageResult*> imageResults_;
    OCRWorker* worker_;
    int totalImages_;
    int completedImages_;
    bool batchComplete_;
    QMutex mutex_;
};

#endif // OCR_CLIENT_H
