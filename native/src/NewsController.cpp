#include "NewsController.h"

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSet>
#include <QXmlStreamReader>

// ---------------------------------------------------------------------------
// NewsModel
// ---------------------------------------------------------------------------

NewsModel::NewsModel(QObject *parent) : QAbstractListModel(parent) {}

int NewsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant NewsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};
    const NewsItem &n = m_items.at(index.row());
    switch (role) {
    case TitleRole: return n.title;
    case LinkRole: return n.link;
    case SummaryRole: return n.summary;
    case PublishedRole: return n.published;
    case SourceRole: return n.source;
    default: return {};
    }
}

QHash<int, QByteArray> NewsModel::roleNames() const
{
    return {
        {TitleRole, "title"},
        {LinkRole, "link"},
        {SummaryRole, "summary"},
        {PublishedRole, "published"},
        {SourceRole, "source"},
    };
}

void NewsModel::setItems(const QVector<NewsItem> &items)
{
    beginResetModel();
    m_items = items;
    endResetModel();
}

NewsItem NewsModel::at(int row) const
{
    if (row < 0 || row >= m_items.size())
        return {};
    return m_items.at(row);
}

// ---------------------------------------------------------------------------
// NewsController
// ---------------------------------------------------------------------------

NewsController::NewsController(QObject *parent) : QObject(parent)
{
    m_net = new QNetworkAccessManager(this);
    m_model = new NewsModel(this);
}

QObject *NewsController::newsModelObject() const { return m_model; }

QVariantMap NewsController::itemAt(int row) const
{
    const NewsItem n = m_model->at(row);
    return {
        {QStringLiteral("title"), n.title},
        {QStringLiteral("link"), n.link},
        {QStringLiteral("summary"), n.summary},
        {QStringLiteral("published"), n.published},
        {QStringLiteral("source"), n.source},
    };
}

void NewsController::refresh()
{
    if (m_busy)
        return;
    m_busy = true;
    emit busyChanged();

    m_collected.clear();
    m_warnings.clear();
    m_jobIndex = 0;
    m_succeededGroup = -1;

    m_jobs = {
        {QStringLiteral("https://archlinux.org/feeds/news/"), QStringLiteral("Arch"),
         kLimit, false, 0},
        {QStringLiteral("https://cachyos.org/blog/index.xml"), QStringLiteral("CachyOS"),
         qMax(3, kLimit / 2), true, 1},
        {QStringLiteral("https://cachyos.org/index.xml"), QStringLiteral("CachyOS"),
         qMax(3, kLimit / 2), true, 1},
    };
    fetchNext();
}

void NewsController::fetchNext()
{
    if (m_jobIndex >= m_jobs.size()) {
        finish();
        return;
    }

    const FeedJob job = m_jobs.at(m_jobIndex);

    // Skip remaining jobs in a fallback group once one succeeded.
    if (job.stopGroupOnSuccess && job.groupId == m_succeededGroup) {
        ++m_jobIndex;
        fetchNext();
        return;
    }

    QNetworkRequest req{QUrl(job.url)};
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("cachyos-updater/1.0"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setTransferTimeout(12000);

    QNetworkReply *reply = m_net->get(req);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, job]() { handleReply(reply, job.source); });
}

void NewsController::handleReply(QNetworkReply *reply, const QString &source)
{
    const FeedJob job = m_jobs.at(m_jobIndex);
    const auto netError = reply->error();
    const QString netErrorString = reply->errorString();

    // Drain the body on every path so Qt can close the SSL socket cleanly.
    const QByteArray data = reply->readAll();
    reply->deleteLater();

    if (netError != QNetworkReply::NoError) {
        // Individual failures inside a fallback group are summarized later.
        if (!job.stopGroupOnSuccess)
            m_warnings << QStringLiteral("Could not fetch %1 news: %2")
                              .arg(source, netErrorString);
    } else {
        QString err;
        const QVector<NewsItem> items = parseFeed(data, source, job.limit, &err);
        if (!items.isEmpty()) {
            m_collected += items;
            if (job.stopGroupOnSuccess)
                m_succeededGroup = job.groupId;
        } else if (!err.isEmpty() && !job.stopGroupOnSuccess) {
            m_warnings << err;
        }
    }

    ++m_jobIndex;
    fetchNext();
}

QVector<NewsItem> NewsController::parseFeed(const QByteArray &data,
                                           const QString &source, int limit,
                                           QString *error)
{
    QVector<NewsItem> items;
    QXmlStreamReader xml(data);

    static const QRegularExpression tagRe(QStringLiteral("<[^>]+>"));
    auto strip = [](QString s) {
        return s.remove(tagRe).simplified();
    };
    auto formatDate = [](const QString &raw) -> QString {
        if (raw.isEmpty())
            return {};
        QDateTime dt = QDateTime::fromString(raw, Qt::RFC2822Date);
        if (!dt.isValid())
            dt = QDateTime::fromString(raw, Qt::ISODate);
        if (dt.isValid())
            return dt.toString(QStringLiteral("yyyy-MM-dd"));
        return raw.left(16);
    };

    while (!xml.atEnd() && !xml.hasError()) {
        const QXmlStreamReader::TokenType token = xml.readNext();
        if (token != QXmlStreamReader::StartElement)
            continue;
        const QString name = xml.name().toString();
        if (name != QLatin1String("item") && name != QLatin1String("entry"))
            continue;

        const bool atom = (name == QLatin1String("entry"));
        NewsItem it;
        it.source = source;

        while (!xml.atEnd()) {
            const QXmlStreamReader::TokenType t = xml.readNext();
            if (t == QXmlStreamReader::EndElement &&
                (xml.name() == QLatin1String("item") ||
                 xml.name() == QLatin1String("entry")))
                break;
            if (t != QXmlStreamReader::StartElement)
                continue;

            const QString tag = xml.name().toString();
            if (tag == QLatin1String("title")) {
                it.title = xml.readElementText(QXmlStreamReader::IncludeChildElements)
                               .simplified();
            } else if (tag == QLatin1String("link")) {
                if (atom) {
                    const QString href =
                        xml.attributes().value(QStringLiteral("href")).toString();
                    if (!href.isEmpty())
                        it.link = href;
                    else
                        it.link = xml.readElementText();
                } else {
                    it.link = xml.readElementText();
                }
            } else if (tag == QLatin1String("description") ||
                       tag == QLatin1String("summary") ||
                       tag == QLatin1String("content")) {
                if (it.summary.isEmpty())
                    it.summary = strip(xml.readElementText(
                                           QXmlStreamReader::IncludeChildElements))
                                     .left(280);
            } else if (tag == QLatin1String("pubDate") ||
                       tag == QLatin1String("updated") ||
                       tag == QLatin1String("published")) {
                if (it.published.isEmpty())
                    it.published = formatDate(xml.readElementText());
            }
        }

        if (!it.title.isEmpty())
            items << it;
        if (items.size() >= limit)
            break;
    }

    if (xml.hasError() && items.isEmpty() && error)
        *error = QStringLiteral("Invalid %1 news feed: %2")
                     .arg(source, xml.errorString());
    else if (items.isEmpty() && error)
        *error = QStringLiteral("No entries parsed from %1 news feed.").arg(source);
    return items;
}

void NewsController::finish()
{
    // De-dupe by title, keep order, cap at limit.
    QSet<QString> seen;
    QVector<NewsItem> unique;
    for (const NewsItem &it : std::as_const(m_collected)) {
        const QString key = it.title.toCaseFolded();
        if (seen.contains(key))
            continue;
        seen.insert(key);
        unique << it;
        if (unique.size() >= kLimit)
            break;
    }

    // Fallback group (CachyOS): collapse per-feed errors into one soft notice.
    if (m_succeededGroup != 1)
        m_warnings << QStringLiteral(
            "CachyOS news feed unavailable right now (Arch news still shown).");

    if (unique.isEmpty() && m_warnings.isEmpty())
        m_warnings << QStringLiteral("No news available right now.");

    m_model->setItems(unique);
    m_busy = false;
    emit busyChanged();
    emit changed();
}
