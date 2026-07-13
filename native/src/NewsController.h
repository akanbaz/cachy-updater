#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QVariantMap>
#include <QVector>

class QNetworkAccessManager;
class QNetworkReply;

struct NewsItem {
    QString title;
    QString link;
    QString summary;
    QString published;
    QString source;
};

class NewsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        LinkRole,
        SummaryRole,
        PublishedRole,
        SourceRole,
    };
    explicit NewsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setItems(const QVector<NewsItem> &items);
    NewsItem at(int row) const;

private:
    QVector<NewsItem> m_items;
};

// Fetches Arch + CachyOS news feeds via QNetworkAccessManager and parses RSS
// 2.0 / Atom with QXmlStreamReader. No curl, no blocking.
class NewsController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString warningText READ warningText NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(QObject *newsModel READ newsModelObject CONSTANT)

public:
    explicit NewsController(QObject *parent = nullptr);

    bool busy() const { return m_busy; }
    QString warningText() const { return m_warnings.join(QStringLiteral("  \u2022  ")); }
    int count() const { return m_model->rowCount(); }
    QObject *newsModelObject() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QVariantMap itemAt(int row) const;

signals:
    void busyChanged();
    void changed();

private:
    void fetchNext();
    void handleReply(QNetworkReply *reply, const QString &source);
    static QVector<NewsItem> parseFeed(const QByteArray &data, const QString &source,
                                       int limit, QString *error);
    void finish();

    struct FeedJob {
        QString url;
        QString source;
        int limit;
        bool stopGroupOnSuccess; // for the CachyOS fallback group
        int groupId;
    };

    QNetworkAccessManager *m_net;
    NewsModel *m_model;
    QVector<NewsItem> m_collected;
    QStringList m_warnings;
    QVector<FeedJob> m_jobs;
    int m_jobIndex = 0;
    int m_succeededGroup = -1;
    bool m_busy = false;
    static constexpr int kLimit = 8;
};
