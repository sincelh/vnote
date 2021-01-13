#include "webviewexporter.h"

#include <QWidget>
#include <QWebEnginePage>
#include <QFileInfo>

#include <widgets/editors/markdownviewer.h>
#include <widgets/editors/editormarkdownvieweradapter.h>
#include <core/editorconfig.h>
#include <core/markdowneditorconfig.h>
#include <core/configmgr.h>
#include <core/htmltemplatehelper.h>
#include <utils/utils.h>
#include <utils/pathutils.h>
#include <utils/fileutils.h>
#include <core/file.h>

using namespace vnotex;

WebViewExporter::WebViewExporter(QWidget *p_parent)
    : QObject(p_parent)
{
}

void WebViewExporter::clear()
{
    m_askedToStop = false;

    delete m_viewer;
    m_viewer = nullptr;
}

bool WebViewExporter::doExport(const ExportOption &p_option,
                               const File *p_file,
                               const QString &p_outputFile)
{
    bool ret = false;
    m_askedToStop = false;

    Q_ASSERT(p_file->getContentType().isMarkdown());

    Q_ASSERT(!m_exportOngoing);
    m_exportOngoing = true;

    m_webViewStates = WebViewState::Started;

    auto viewer = getWebViewer();
    const auto htmlTemplate =
        HtmlTemplateHelper::generateMarkdownViewerTemplate(ConfigMgr::getInst().getEditorConfig().getMarkdownEditorConfig(),
                                                           p_option.m_renderingStyleFile,
                                                           p_option.m_syntaxHighlightStyleFile);
    viewer->setHtml(htmlTemplate, PathUtils::pathToUrl(p_file->getContentPath()));
    viewer->adapter()->setText(p_file->read());

    while (!isWebViewReady()) {
        Utils::sleepWait(100);

        if (m_askedToStop) {
            goto exit_export;
        }

        if (isWebViewFailed()) {
            qWarning() << "WebView failed when exporting" << p_file->getFilePath();
            goto exit_export;
        }
    }

    qDebug() << "WebView is ready";

    // Add extra wait to make sure Web side is really ready.
    Utils::sleepWait(200);

    switch (p_option.m_targetFormat) {
    case ExportFormat::HTML:
        Q_ASSERT(p_option.m_htmlOption);
        // TODO: not supported yet.
        Q_ASSERT(!p_option.m_htmlOption->m_useMimeHtmlFormat);
        ret = doExportHtml(*p_option.m_htmlOption, p_outputFile);
        break;

    default:
        break;
    }

exit_export:
    m_exportOngoing = false;
    return ret;
}

void WebViewExporter::stop()
{
    m_askedToStop = true;
}

MarkdownViewer *WebViewExporter::getWebViewer()
{
    if (!m_viewer) {
        // Adapter will be managed by MarkdownViewer.
        auto adapter = new MarkdownViewerAdapter(this);
        m_viewer = new MarkdownViewer(adapter, QColor(), 1, static_cast<QWidget *>(parent()));
        m_viewer->hide();
        connect(m_viewer->page(), &QWebEnginePage::loadFinished,
                this, [this]() {
                    m_webViewStates |= WebViewState::LoadFinished;
                });
        connect(adapter, &MarkdownViewerAdapter::workFinished,
                this, [this]() {
                    m_webViewStates |= WebViewState::WorkFinished;
                });
    }

    return m_viewer;
}

bool WebViewExporter::isWebViewReady() const
{
    return m_webViewStates == (WebViewState::LoadFinished | WebViewState::WorkFinished);
}

bool WebViewExporter::isWebViewFailed() const
{
    return m_webViewStates & WebViewState::Failed;
}

bool WebViewExporter::doExportHtml(const ExportHtmlOption &p_htmlOption,
                                   const QString &p_outputFile)
{
    // 0 - Busy
    // 1 - Finished
    // -1 - Failed
    int state = 0;

    connect(m_viewer->adapter(), &MarkdownViewerAdapter::contentReady,
            this, [&, this](const QString &p_headContent, const QString &p_styleContent, const QString &p_bodyContent) {
                qDebug() << "doExportHtml contentReady";
                // Maybe unnecessary. Just to avoid duplicated signal connections.
                disconnect(m_viewer->adapter(), &MarkdownViewerAdapter::contentReady, this, 0);

                if (p_bodyContent.isEmpty() || m_askedToStop) {
                    state = -1;
                    return;
                }

                if (!writeHtmlFile(p_outputFile,
                                   p_headContent,
                                   p_styleContent,
                                   p_bodyContent,
                                   p_htmlOption.m_embedStyles,
                                   p_htmlOption.m_completePage,
                                   p_htmlOption.m_embedImages)) {
                    state = -1;
                    return;
                }

                state = 1;
            });

    m_viewer->adapter()->saveContent();


    while (state == 0) {
        Utils::sleepWait(100);

        if (m_askedToStop) {
            break;
        }
    }

    return state == 1;
}

bool WebViewExporter::writeHtmlFile(const QString &p_file,
                                    const QString &p_headContent,
                                    const QString &p_styleContent,
                                    const QString &p_bodyContent,
                                    bool p_embedStyles,
                                    bool p_completePage,
                                    bool p_embedImages)
{
    const auto baseName = QFileInfo(p_file).completeBaseName();
    auto title = QString("%1 - %2").arg(baseName, ConfigMgr::c_appName);
    QString resourceFolder = baseName + "_files";
    resourceFolder = PathUtils::concatenateFilePath(PathUtils::parentDirPath(p_file), resourceFolder);

    qDebug() << "HTML files folder" << resourceFolder;

    /*
    QString html(m_exportHtmlTemplate);
    if (!p_title.isEmpty()) {
        html.replace(HtmlHolder::c_headTitleHolder,
                     "<title>" + VUtils::escapeHtml(p_title) + "</title>");
    }

    if (!p_styleContent.isEmpty() && p_embedCssStyle) {
        QString content(p_styleContent);
        embedStyleResources(content);
        html.replace(HtmlHolder::c_styleHolder, content);
    }

    if (!p_headContent.isEmpty()) {
        html.replace(HtmlHolder::c_headHolder, p_headContent);
    }

    if (p_completeHTML) {
        QString content(p_bodyContent);
        if (p_embedImages) {
            embedBodyResources(m_baseUrl, content);
        } else {
            fixBodyResources(m_baseUrl, resourceFolder, content);
        }

        html.replace(HtmlHolder::c_bodyHolder, content);
    } else {
        html.replace(HtmlHolder::c_bodyHolder, p_bodyContent);
    }

    file.write(html.toUtf8());
    file.close();

    // Delete empty resource folder.
    QDir dir(resourceFolder);
    if (dir.isEmpty()) {
        dir.cdUp();
        dir.rmdir(resFolder);
    }

    return true;
    */
    return false;
}
