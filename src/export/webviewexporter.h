#ifndef WEBVIEWEXPORTER_H
#define WEBVIEWEXPORTER_H

#include <QObject>

#include "exportdata.h"

class QWidget;

namespace vnotex
{
    class File;
    class MarkdownViewer;

    class WebViewExporter : public QObject
    {
        Q_OBJECT
    public:
        enum WebViewState
        {
            Started = 0,
            LoadFinished = 0x1,
            WorkFinished = 0x2,
            Failed = 0x4
        };
        Q_DECLARE_FLAGS(WebViewStates, WebViewState);

        // We need QWidget as parent.
        explicit WebViewExporter(QWidget *p_parent);

        bool doExport(const ExportOption &p_option,
                      const File *p_file,
                      const QString &p_outputFile);

        // Release resources after one batch of export.
        void clear();

        void stop();

    signals:
        void logRequested(const QString &p_log);

    private:
        MarkdownViewer *getWebViewer();

        bool isWebViewReady() const;

        bool isWebViewFailed() const;

        bool doExportHtml(const ExportHtmlOption &p_htmlOption,
                          const QString &p_outputFile);

        bool writeHtmlFile(const QString &p_file,
                           const QString &p_headContent,
                           const QString &p_styleContent,
                           const QString &p_bodyContent,
                           bool p_embedStyles,
                           bool p_completePage,
                           bool p_embedImages);

        bool m_askedToStop = false;

        bool m_exportOngoing = false;

        WebViewStates m_webViewStates = WebViewState::Started;

        // Managed by QObject.
        MarkdownViewer *m_viewer = nullptr;
    };
}

Q_DECLARE_OPERATORS_FOR_FLAGS(vnotex::WebViewExporter::WebViewStates)

#endif // WEBVIEWEXPORTER_H
