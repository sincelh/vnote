#include "htmltemplatehelper.h"

#include <QDebug>

#include <core/markdowneditorconfig.h>
#include <core/configmgr.h>
#include <utils/utils.h>
#include <utils/fileutils.h>
#include <utils/pathutils.h>
#include <core/thememgr.h>
#include <core/vnotex.h>

using namespace vnotex;

HtmlTemplateHelper::Template HtmlTemplateHelper::s_markdownViewerTemplate;

QString WebGlobalOptions::toJavascriptObject() const
{
    return QStringLiteral("window.vxOptions = {\n")
           + QString("webPlantUml: %1,\n").arg(Utils::boolToString(m_webPlantUml))
           + QString("webGraphviz: %1,\n").arg(Utils::boolToString(m_webGraphviz))
           + QString("constrainImageWidthEnabled: %1,\n").arg(Utils::boolToString(m_constrainImageWidthEnabled))
           + QString("protectFromXss: %1,\n").arg(Utils::boolToString(m_protectFromXss))
           + QString("htmlTagEnabled: %1,\n").arg(Utils::boolToString(m_htmlTagEnabled))
           + QString("autoBreakEnabled: %1,\n").arg(Utils::boolToString(m_autoBreakEnabled))
           + QString("linkifyEnabled: %1,\n").arg(Utils::boolToString(m_linkifyEnabled))
           + QString("indentFirstLineEnabled: %1,\n").arg(Utils::boolToString(m_indentFirstLineEnabled))
           + QString("sectionNumberEnabled: %1,\n").arg(Utils::boolToString(m_sectionNumberEnabled))
           + QString("sectionNumberBaseLevel: %1\n").arg(m_sectionNumberBaseLevel)
           + QStringLiteral("}");
}

static bool isGlobalStyles(const ViewerResource::Resource &p_resource)
{
    return p_resource.m_name == QStringLiteral("global_styles");
}

// Read "global_styles" from resource and fill the holder with the content.
static void fillGlobalStyles(QString &p_template, const ViewerResource &p_resource)
{
    QString styles;
    for (const auto &ele : p_resource.m_resources) {
        if (isGlobalStyles(ele)) {
            if (ele.m_enabled) {
                for (const auto &style : ele.m_styles) {
                    // Read the style file content.
                    auto styleFile = ConfigMgr::getInst().getUserOrAppFile(style);
                    styles += FileUtils::readTextFile(styleFile);
                }
            }
            break;
        }
    }

    if (!styles.isEmpty()) {
        p_template.replace(QStringLiteral("/* VX_GLOBAL_STYLES_PLACEHOLDER */"),
                           styles);
    }
}

static QString fillStyleTag(const QString &p_styleFile)
{
    if (p_styleFile.isEmpty()) {
        return "";
    }
    auto url = PathUtils::pathToUrl(p_styleFile);
    return QString("<link rel=\"stylesheet\" type=\"text/css\" href=\"%1\">\n").arg(url.toString());
}

static QString fillScriptTag(const QString &p_scriptFile)
{
    if (p_scriptFile.isEmpty()) {
        return "";
    }
    auto url = PathUtils::pathToUrl(p_scriptFile);
    return QString("<script type=\"text/javascript\" src=\"%1\"></script>\n").arg(url.toString());
}

static void fillThemeStyles(QString &p_template, const QString &p_webStyleSheetFile, const QString &p_highlightStyleSheetFile)
{
    QString styles;
    styles += fillStyleTag(p_webStyleSheetFile);
    styles += fillStyleTag(p_highlightStyleSheetFile);

    if (!styles.isEmpty()) {
        p_template.replace(QStringLiteral("<!-- VX_THEME_STYLES_PLACEHOLDER -->"),
                           styles);
    }
}

static void fillGlobalOptions(QString &p_template, const WebGlobalOptions &p_opts)
{
    p_template.replace(QStringLiteral("/* VX_GLOBAL_OPTIONS_PLACEHOLDER */"),
                       p_opts.toJavascriptObject());
}

// Read all other resources in @p_resource and fill the holder with proper resource path.
static void fillResources(QString &p_template, const ViewerResource &p_resource)
{
    QString styles;
    QString scripts;

    for (const auto &ele : p_resource.m_resources) {
        if (ele.m_enabled && !isGlobalStyles(ele)) {
            // Styles.
            for (const auto &style : ele.m_styles) {
                auto styleFile = ConfigMgr::getInst().getUserOrAppFile(style);
                styles += fillStyleTag(styleFile);
            }

            // Scripts.
            for (const auto &script : ele.m_scripts) {
                auto scriptFile = ConfigMgr::getInst().getUserOrAppFile(script);
                scripts += fillScriptTag(scriptFile);
            }
        }
    }

    if (!styles.isEmpty()) {
        p_template.replace(QStringLiteral("<!-- VX_STYLES_PLACEHOLDER -->"),
                           styles);
    }

    if (!scripts.isEmpty()) {
        p_template.replace(QStringLiteral("<!-- VX_SCRIPTS_PLACEHOLDER -->"),
                           scripts);
    }
}

const QString &HtmlTemplateHelper::getMarkdownViewerTemplate()
{
    return s_markdownViewerTemplate.m_template;
}

void HtmlTemplateHelper::updateMarkdownViewerTemplate(const MarkdownEditorConfig &p_config)
{
    if (p_config.revision() == s_markdownViewerTemplate.m_revision) {
        return;
    }

    s_markdownViewerTemplate.m_revision = p_config.revision();

    const auto &themeMgr = VNoteX::getInst().getThemeMgr();
    s_markdownViewerTemplate.m_template =
        generateMarkdownViewerTemplate(p_config,
                                       themeMgr.getFile(Theme::File::WebStyleSheet),
                                       themeMgr.getFile(Theme::File::HighlightStyleSheet));
}

QString HtmlTemplateHelper::generateMarkdownViewerTemplate(const MarkdownEditorConfig &p_config,
                                                           const QString &p_webStyleSheetFile,
                                                           const QString &p_highlightStyleSheetFile)
{
    const auto &viewerResource = p_config.getViewerResource();

    const auto templateFile = ConfigMgr::getInst().getUserOrAppFile(viewerResource.m_template);
    auto htmlTemplate = FileUtils::readTextFile(templateFile);

    fillGlobalStyles(htmlTemplate, viewerResource);

    fillThemeStyles(htmlTemplate, p_webStyleSheetFile, p_highlightStyleSheetFile);

    {
        WebGlobalOptions opts;
        opts.m_webPlantUml = p_config.getWebPlantUml();
        opts.m_webGraphviz = p_config.getWebGraphviz();
        opts.m_sectionNumberEnabled = p_config.getSectionNumberMode() == MarkdownEditorConfig::SectionNumberMode::Read;
        opts.m_sectionNumberBaseLevel = p_config.getSectionNumberBaseLevel();
        opts.m_constrainImageWidthEnabled = p_config.getConstrainImageWidthEnabled();
        opts.m_protectFromXss = p_config.getProtectFromXss();
        opts.m_htmlTagEnabled = p_config.getHtmlTagEnabled();
        opts.m_autoBreakEnabled = p_config.getAutoBreakEnabled();
        opts.m_linkifyEnabled = p_config.getLinkifyEnabled();
        opts.m_indentFirstLineEnabled = p_config.getIndentFirstLineEnabled();
        fillGlobalOptions(htmlTemplate, opts);
    }

    fillResources(htmlTemplate, viewerResource);

    return htmlTemplate;
}
