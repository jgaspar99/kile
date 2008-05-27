/***************************************************************************
    begin                : Fri Aug 1 2003
    copyright            : (C) 2003 by Jeroen Wijnhout
                         : (C) 2007  by Holger Danielsson
    email                : Jeroen.Wijnhout@kdemail.net
                           holger.danielsson@versanet.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// 2007-03-12 dani
//  - use KileDocument::Extensions
//  - allowed extensions are always defined as list, f.e.: .tex .ltx .latex 

#include "kileproject.h"
#include "kileversion.h"

#include <qstringlist.h>
#include <qfileinfo.h>
#include <qdir.h>

#include <klocale.h>
#include <kglobal.h>
#include "kiledebug.h"
#include <kmessagebox.h>
#include <ksharedptr.h>

#include "documentinfo.h"
#include "kiledocmanager.h"
#include "kiletoolmanager.h"
#include "kileinfo.h"
#include "kileextensions.h"
#include <krun.h>
/*
 01/24/06 tbraun
	Added the logic to get versioned kilepr files
	We also warn if the user wants to open a project file with a different kileprversion
*/


/*
 * KileProjectItem
 */
KileProjectItem::KileProjectItem(KileProject *project, const KUrl & url, int type) :
	m_project(project),
	m_url(url),
	m_type(type),
	m_docinfo(0L),
	m_parent(0L),
	m_child(0L),
	m_sibling(0L),
	m_nLine(0),
	m_order(-1)
{
	m_highlight=m_encoding=QString::null;
	m_bOpen = m_archive = true;

	if (project)
		project->add(this);
}

void KileProjectItem::setOrder(int i)
{
	m_order = i;
}

void KileProjectItem::setParent(KileProjectItem * item)
{
	m_parent = item;

	//update parent info
	if (m_parent)
	{
		if (m_parent->firstChild())
		{
			//get last child
			KileProjectItem *sib = m_parent->firstChild();
			while (sib->sibling())
				sib = sib->sibling();

			sib->setSibling(this);
		}
		else
			m_parent->setChild(this);
	}
	else
	{
		setChild(0);
		setSibling(0);
	}
}

void KileProjectItem::print(int level)
{
	QString str;
	str.fill('\t', level);
	KILE_DEBUG() << str << "+" << url().fileName();

	if (firstChild())
		firstChild()->print(++level);

	if (sibling())
		sibling()->print(level);
}

void KileProjectItem::allChildren(QList<KileProjectItem*> *list) const
{
	KileProjectItem *item = firstChild();

// 	KILE_DEBUG() << "\tKileProjectItem::allChildren(" << list->count() << ")";
	while(item != NULL) {
		list->append(item);
// 		KILE_DEBUG() << "\t\tappending " << item->url().fileName();
		item->allChildren(list);
		item = item->sibling();
	}
}

void KileProjectItem::setInfo(KileDocument::TextInfo *docinfo)
{
	m_docinfo = docinfo;
	if(docinfo)
	{
	connect(docinfo,SIGNAL(urlChanged(KileDocument::Info*, const KUrl &)), this, SLOT(slotChangeURL(KileDocument::Info*, const KUrl &)));
	connect(docinfo,SIGNAL(depChanged()), m_project, SLOT(buildProjectTree()));
	}
}

void KileProjectItem::changeURL(const KUrl &url)
{
	// don't allow empty URLs
	if(!url.isEmpty() && m_url != url) 
	{
		m_url = url;
		emit(urlChanged(this));
	}
}

void KileProjectItem::slotChangeURL(KileDocument::Info*, const KUrl &url)
{
	changeURL(url);
}

/*
 * KileProject
 */
KileProject::KileProject(const QString& name, const KUrl& url, KileDocument::Extensions *extensions) : QObject(0,name.ascii()), m_invalid(false), m_masterDocument(QString::null), m_useMakeIndexOptions(false)
{
	init(name, url, extensions);
}

KileProject::KileProject(const KUrl& url, KileDocument::Extensions *extensions) : QObject(0,url.fileName().ascii()), m_invalid(false), m_masterDocument(QString::null), m_useMakeIndexOptions(false)
{
	init(url.fileName(), url, extensions);
}	

KileProject::~KileProject()
{
	KILE_DEBUG() << "DELETING KILEPROJECT " <<  m_projecturl.url();
	delete m_config;

	for(QList<KileProjectItem*>::iterator it = m_projectItems.begin(); it != m_projectItems.end(); ++it) {
		delete *it;
	}
}

void KileProject::init(const QString& name, const KUrl& url, KileDocument::Extensions *extensions)
{
	m_name = name;
	m_projecturl = KileDocument::Manager::symlinkFreeURL( url);;

	m_config = new KConfig(m_projecturl.path(), KConfig::SimpleConfig);
	m_extmanager = extensions;

	m_baseurl = m_projecturl.directory();
	m_baseurl.cleanPath(KUrl::SimplifyDirSeparators);

	KILE_DEBUG() << "KileProject m_baseurl = " << m_baseurl.path();

	if (QFileInfo(url.path()).exists())
	{
		load();
	}
	else
	{
		//create the project file
		KConfigGroup configGroup = m_config->group("General");
		configGroup.writeEntry("name", m_name);
		configGroup.writeEntry("kileprversion", kilePrVersion);
		configGroup.writeEntry("kileversion", kileVersion);
		configGroup.sync();
	}
}

void KileProject::setLastDocument(const KUrl &url)
{
    if ( item(url) != 0 )
        m_lastDocument = KileDocument::Manager::symlinkFreeURL(url);
}

void KileProject::setExtensions(KileProjectItem::Type type, const QString & ext)
{
	if (type<KileProjectItem::Source || type>KileProjectItem::Image) 
	{
		kWarning() << "ERROR: TYPE<1 or TYPE>3";
		return;
	}

	// first we take all standard extensions
	QStringList standardExtList;
	if ( type == KileProjectItem::Source )
		standardExtList = (m_extmanager->latexDocuments()).split(" ");
	else if ( type == KileProjectItem::Package )
		standardExtList = (m_extmanager->latexPackages()).split(" ");
	else // if ( type == KileProjectItem::Image )
		standardExtList = (m_extmanager->images()).split(" ");

	// now we scan user defined list and accept all extension, 
	// except standard extensions of course
	QString userExt;
	if ( ! ext.isEmpty() )
	{
		QStringList userExtList;

		QStringList::ConstIterator it;
		QStringList list = ext.split(" ");
		for ( it=list.begin(); it != list.end(); ++it ) 
		{
			// some tiny extension checks
			if ( (*it).length()<2 || (*it)[0]!='.' )
				continue;

			// some of the old definitions are wrong, so we test them all
			if ( type==KileProjectItem::Source || type==KileProjectItem::Package)
			{
				if ( ! (m_extmanager->isLatexDocument(*it) || m_extmanager->isLatexPackage(*it)) )
				{
					standardExtList << (*it);
					userExtList << (*it);
				}
			}
			else // if ( type == KileProjectItem::Image )
			{
				if ( ! m_extmanager->isImage(*it) )
				{
					standardExtList << (*it);
					userExtList << (*it);
				}
			}
		}
		if ( userExtList.count() > 0 )
			userExt = userExtList.join(" ");	
	}

	// now we build a regular expression for all extensions
	// (used to search for a filename with a valid extension)
	QString pattern = standardExtList.join("|");
	pattern.replace(".","\\.");
	pattern = '('+ pattern +")$";

	// and save it
	m_reExtensions[type-1].setPattern(pattern);

	// if the list of user defined extensions has changed
	// we save the new value and (re)build the project tree
	if (m_extensions[type-1] != userExt)
	{
		m_extensions[type-1] = userExt;
		buildProjectTree();
	}
}

void KileProject::setType(KileProjectItem *item)
{
	if ( item->path().right(7) == ".kilepr" )
	{
		item->setType(KileProjectItem::ProjectFile);
		return;
	}

	bool unknown = true;
	for (int i = KileProjectItem::Source; i < KileProjectItem::Other; ++i)
	{
		if ( m_reExtensions[i-1].search(item->url().fileName()) != -1)
		{
			item->setType(i);
			unknown = false;
			break;
		}
	}

	if (unknown)
		item->setType(KileProjectItem::Other);
}

void KileProject::readMakeIndexOptions()
{
	QString grp = KileTool::groupFor("MakeIndex", m_config);

	//get the default value
	KSharedConfig::Ptr cfg = KGlobal::config();
	KConfigGroup configGroup = cfg->group(KileTool::groupFor("MakeIndex", KileTool::configName("MakeIndex", cfg.data())));
	QString deflt = configGroup.readEntry("options", "'%S'.idx");
	
	if ( useMakeIndexOptions() && !grp.isEmpty() )
	{
		KConfigGroup makeIndexGroup = m_config->group(grp);
		QString val = makeIndexGroup.readEntry("options", deflt);
		if ( val.isEmpty() ) val = deflt;
		setMakeIndexOptions(val);
	}
	else //use default value
		setMakeIndexOptions(deflt);
}

void KileProject::writeUseMakeIndexOptions()
{
	if ( useMakeIndexOptions() )
		KileTool::setConfigName("MakeIndex", "Default", m_config);
	else 
		KileTool::setConfigName("MakeIndex", "", m_config);
}

QString KileProject::addBaseURL(const QString &path)
{
	KILE_DEBUG() << "===addBaseURL(const QString & " << path << " )";
	if ( path.isEmpty())
		return path;
  	else if ( path.startsWith("/") )
  		return KileDocument::Manager::symlinkFreeURL(KUrl::fromPathOrUrl(path)).path();
  	else
    		return  KileDocument::Manager::symlinkFreeURL(KUrl::fromPathOrUrl(m_baseurl.path() + '/' +path)).path();
 }

QString KileProject::removeBaseURL(const QString &path)
{

  if ( path.startsWith("/") )
  {
    QFileInfo info(path);
    QString relPath = findRelativePath(path);
    KILE_DEBUG() << "removeBaseURL path is" << path << " , relPath is " << relPath;
    return relPath;
  }
  else
  {
    return path;
  }
}

bool KileProject::load()
{
	KILE_DEBUG() << "KileProject: loading..." <<endl;

	//load general settings/options
	KConfigGroup generalGroup = m_config->group("General");
	m_name = generalGroup.readEntry("name", i18n("Project"));
	m_kileversion = generalGroup.readEntry("kileversion", QString());
	m_kileprversion = generalGroup.readEntry("kileprversion",QString());

	if(!m_kileprversion.isNull() && m_kileprversion.toInt() > kilePrVersion.toInt())
	{
		if(KMessageBox::warningYesNo(NULL, i18n("The project file of %1 was created by a newer version of kile.\
				Opening it can lead to unexpected results.\n\
				Do you really want to continue (not recommended)?", m_name),
				 QString::null, KStandardGuiItem::yes(), KStandardGuiItem::no(), QString(), KMessageBox::Dangerous) == KMessageBox::No)
		{
			m_invalid=true;
			return false;
		}
	}
	
	QString master = addBaseURL(generalGroup.readEntry("masterDocument", QString()));
  	KILE_DEBUG() << "masterDoc == " << master;
	setMasterDocument(master);

	setExtensions(KileProjectItem::Source, generalGroup.readEntry("src_extensions",m_extmanager->latexDocuments()));
	setExtensions(KileProjectItem::Package, generalGroup.readEntry("pkg_extensions",m_extmanager->latexPackages()));
	setExtensions(KileProjectItem::Image, generalGroup.readEntry("img_extensions",m_extmanager->images()));

	setQuickBuildConfig(KileTool::configName("QuickBuild", m_config));

	if( KileTool::configName("MakeIndex",m_config).compare("Default") == 0)
		setUseMakeIndexOptions(true);
	else
		setUseMakeIndexOptions(false);

	readMakeIndexOptions();

	KUrl url;
	KileProjectItem *item;
	QStringList groups = m_config->groupList();

	//retrieve all the project files and create and initialize project items for them
	for (int i = 0; i < groups.count(); ++i) {
		if (groups[i].left(5) == "item:") {
			QString path = groups[i].mid(5);
			if (path[0] == '/' ) {
				url = KUrl::fromPathOrUrl(path);
			}
			else {
				url = m_baseurl;
				url.addPath(path);
				url.cleanPath(KUrl::SimplifyDirSeparators);
			}
			item = new KileProjectItem(this, KileDocument::Manager::symlinkFreeURL(url));
			setType(item);

			KConfigGroup configGroup = m_config->group(groups[i]);
			item->setOpenState(configGroup.readEntry("open", true));
			item->setEncoding(configGroup.readEntry("encoding", QString()));
			item->setHighlight(configGroup.readEntry("highlight", QString()));
			item->setArchive(configGroup.readEntry("archive", true));
			item->setLineNumber(configGroup.readEntry("line", 0));
			item->setColumnNumber(configGroup.readEntry("column", 0));
			item->setOrder(configGroup.readEntry("order", -1));
			item->changePath(groups[i].mid(5));

			connect(item, SIGNAL(urlChanged(KileProjectItem*)), this, SLOT(itemRenamed(KileProjectItem*)) );
		}
	}

    // only call this after all items are created, otherwise setLastDocument doesn't accept the url
    generalGroup = m_config->group("General");
    setLastDocument(KUrl::fromPathOrUrl(addBaseURL(generalGroup.readEntry("lastDocument", QString()))));

// 	dump();

	return true;
}

bool KileProject::save()
{
	KILE_DEBUG() << "KileProject: saving..." <<endl;

	KConfigGroup generalGroup = m_config->group("General");
	generalGroup.writeEntry("name", m_name);
	generalGroup.writeEntry("kileprversion", kilePrVersion);
	generalGroup.writeEntry("kileversion", kileVersion);

	KILE_DEBUG() << "KileProject::save() masterDoc = " << removeBaseURL(m_masterDocument);
	generalGroup.writeEntry("masterDocument", removeBaseURL(m_masterDocument));
	generalGroup.writeEntry("lastDocument", removeBaseURL(m_lastDocument.path()));


	writeConfigEntry("src_extensions",m_extmanager->latexDocuments(),KileProjectItem::Source);
	writeConfigEntry("pkg_extensions",m_extmanager->latexPackages(),KileProjectItem::Package);
	writeConfigEntry("img_extensions",m_extmanager->images(),KileProjectItem::Image);
	// only to avoid problems with older versions
	generalGroup.writeEntry("src_extIsRegExp", false);
	generalGroup.writeEntry("pkg_extIsRegExp", false);
	generalGroup.writeEntry("img_extIsRegExp", false);

	for(QList<KileProjectItem*>::iterator it = m_projectItems.begin(); it != m_projectItems.end(); ++it) {
		KileProjectItem *item = *it;
		KConfigGroup itemGroup = m_config->group("item:" + item->path());
		itemGroup.writeEntry("open", item->isOpen());
		itemGroup.writeEntry("encoding", item->encoding());
		itemGroup.writeEntry("highlight", item->highlight());
		itemGroup.writeEntry("archive", item->archive());
		itemGroup.writeEntry("line", item->lineNumber());
		itemGroup.writeEntry("column", item->columnNumber());
		itemGroup.writeEntry("order", item->order());
	}

	KileTool::setConfigName("QuickBuild", quickBuildConfig(), m_config);

	writeUseMakeIndexOptions();
	if(useMakeIndexOptions()) {
		QString grp = KileTool::groupFor("MakeIndex", m_config);
		if(grp.isEmpty()) {
			grp = "Default";
		}
		KConfigGroup configGroup = m_config->group(grp);
		configGroup.writeEntry("options", makeIndexOptions());
	}

	m_config->sync();

	// dump();

	return true;
}

void KileProject::writeConfigEntry(const QString &key, const QString &standardExt, KileProjectItem::Type type)
{
	QString userExt = extensions(type);
	if ( userExt.isEmpty() )
		m_config->group(QString()).writeEntry(key,standardExt);
	else
		m_config->group(QString()).writeEntry(key,standardExt + ' ' + extensions(type));
}

void KileProject::buildProjectTree()
{
	KILE_DEBUG() << "==KileProject::buildProjectTree==========================";

	//determine the parent doc for each item (TODO:an item can only have one parent, not necessarily true for LaTeX docs)

	const QStringList *deps;
	QString dep;
	KileProjectItem *itm;
	KUrl url;

	//clean first
	for(QList<KileProjectItem*>::iterator it = m_projectItems.begin(); it != m_projectItems.end(); ++it) {
		(*it)->setParent(0);
	}

	//use the dependencies list of the documentinfo object to determine the parent
	for(QList<KileProjectItem*>::iterator it = m_projectItems.begin(); it != m_projectItems.end(); ++it) {
		//set the type correctly (changing m_extensions causes a call to buildProjectTree)
		setType(*it);

		if((*it)->getInfo()) {
			deps = (*it)->getInfo()->dependencies();
			for(int i = 0; i < deps->count(); ++i) {
				dep = (*deps)[i];

				if( m_extmanager->isTexFile(dep) )
					url = KileInfo::checkOtherPaths(m_baseurl,dep,KileInfo::texinputs);
				else if( m_extmanager->isBibFile(dep) )
					url = KileInfo::checkOtherPaths(m_baseurl,dep,KileInfo::bibinputs);
				
				itm = item(url);
				if (itm && (itm->parent() == 0))
					itm->setParent(*it);
			}
		}
	}

	//make a list of all the root items (items with parent == 0)
	m_rootItems.clear();
	for(QList<KileProjectItem*>::iterator it = m_projectItems.begin(); it != m_projectItems.end(); ++it) {
		if((*it)->parent() == NULL) {
			m_rootItems.append(*it);
		}
	}

	emit(projectTreeChanged(this));
}

KileProjectItem* KileProject::item(const KUrl& url)
{
	for(QList<KileProjectItem*>::iterator it = m_projectItems.begin(); it != m_projectItems.end(); ++it) {
		if((*it)->url() == url) {
			return *it;
		}
	}

	return NULL;
}

KileProjectItem* KileProject::item(const KileDocument::Info *info)
{
	for(QList<KileProjectItem*>::iterator it = m_projectItems.begin(); it != m_projectItems.end(); ++it) {
		KileProjectItem *current = *it;

		if (current->getInfo() == info) {
			return current;
		}
	}

	return NULL;
}

void KileProject::add(KileProjectItem* item)
{
	KILE_DEBUG() << "KileProject::add projectitem" << item->url().path();

	setType(item);

	item->changePath(findRelativePath(item->url()));
	connect(item, SIGNAL(urlChanged(KileProjectItem*)), this, SLOT(itemRenamed(KileProjectItem*)) );

	m_projectItems.append(item);

	// dump();
}

void KileProject::remove(KileProjectItem* item)
{
	if(m_config->hasGroup("item:" + item->path())) {
		m_config->deleteGroup("item:" + item->path());
	}
	else {
		kWarning() << "KileProject::remove() Failed to delete the group corresponding to this item!!!";
	}

	KILE_DEBUG() << "KileProject::remove";
	m_projectItems.remove(item);

	// dump();
}

void KileProject::itemRenamed(KileProjectItem *item)
{
	KILE_DEBUG() << "==KileProject::itemRenamed==========================";
	KILE_DEBUG() << "\t" << item->url().fileName();
	m_config->deleteGroup("item:"+item->path());
	//config.sync();

	item->changePath(findRelativePath(item->url()));
}

QString KileProject::findRelativePath(const KUrl &url)
{
	QString basepath = m_baseurl.path();
	QString path = url.directory();
	QString filename = url.fileName();

 	KILE_DEBUG() <<"===findRelativeURL==================";
 	KILE_DEBUG() << "\tbasepath : " <<  basepath << " path: " << path;

//   if ( basepath == path )
//   {
//     return "./";
//   }

	QStringList basedirs = basepath.split("/", QString::SkipEmptyParts);
	QStringList dirs = path.split("/", QString::SkipEmptyParts);

	int nDirs = dirs.count();
	//uint nBaseDirs = basedirs.count();

// 	for (uint i=0; i < basedirs.count(); ++i)
// 	{
// 		KILE_DEBUG() << "\t\tbasedirs " << i << ": " << basedirs[i];
// 	}

// 	for (uint i=0; i < dirs.count(); ++i)
// 	{
//  		KILE_DEBUG() << "\t\tdirs " << i << ": " << dirs[i];
// 	}

	while ( dirs.count() > 0 && basedirs.count() > 0 &&  dirs[0] == basedirs[0] ) {
		dirs.pop_front();
		basedirs.pop_front();
	}

// 	KILE_DEBUG() << "\tafter";
// 	for (uint i=0; i < basedirs.count(); ++i)
// 	{
// 		KILE_DEBUG() << "\t\tbasedirs " << i << ": " << basedirs[i];
// 	}
// 
// 	for (uint i=0; i < dirs.count(); ++i)
// 	{
// 		KILE_DEBUG() << "\t\tdirs " << i << ": " << dirs[i];
// 	}

	if(nDirs != dirs.count()) {
		path = dirs.join("/");

		if (basedirs.count() > 0) {
			for (int j = 0; j < basedirs.count(); ++j) {
				path = "../" + path;
			}
		}

		if ( path.length() > 0 && path.right(1) != "/" ) path = path + '/';

		path = path+filename;
	}
	else { //assume an absolute path was requested
		path = url.path();
	}

//  	KILE_DEBUG() << "\tpath : " << path;

	return path;
}

bool KileProject::contains(const KUrl &url)
{
	for(QList<KileProjectItem*>::iterator it = m_projectItems.begin(); it != m_projectItems.end(); ++it) {
		if((*it)->url() == url) {
			return true;
		}
	}

	return false;
}

bool KileProject::contains(const KileDocument::Info *info)
{
	for(QList<KileProjectItem*>::iterator it = m_projectItems.begin(); it != m_projectItems.end(); ++it) {
		if((*it)->getInfo() == info) {
			return true;
		}
	}
	return false;
}

KileProjectItem *KileProject::rootItem(KileProjectItem *item) const
{
	//find the root item (i.e. the eldest parent)
	KileProjectItem *root = item;
	while(root->parent() != NULL) {
		root = root->parent();
	}

	//check if this root item is a LaTeX root
	if(root->getInfo()) {
		if (root->getInfo()->isLaTeXRoot()) {
			return root;
		}
		else {
			//if not, see if we can find another root item that is a LaTeX root
			for(QList<KileProjectItem*>::const_iterator it = m_rootItems.begin(); it != m_rootItems.end(); ++it) {
				KileProjectItem *current = *it;
				if(current->getInfo() && current->getInfo()->isLaTeXRoot()) {
					return current;
				}
			}
		}

		//no LaTeX root found, return previously found root
		return root;
	}
	
	//root is not a valid item (getInfo() return 0L), return original item	
	return item;
}

void KileProject::dump()
{
	KILE_DEBUG() << "KileProject::dump() " << m_name;
	for(QList<KileProjectItem*>::iterator it = m_projectItems.begin(); it != m_projectItems.end(); ++it) {
		KileProjectItem *item = *it;
		KILE_DEBUG() << "item " << item << " has path: "  << item->path();
		KILE_DEBUG() << "item->type() " << item->type();
		KILE_DEBUG() << "OpenState: " << item->isOpen();
	}
}

QString KileProject::archiveFileList() const
{
	KILE_DEBUG() << "KileProject::archiveFileList()";

	QString path, list;
	for(QList<KileProjectItem*>::const_iterator it = m_projectItems.begin(); it != m_projectItems.end(); ++it) {
		if ((*it)->archive()) {
			path = (*it)->path();
			KRun::shellQuote(path);
			list.append(path + ' ');
		}
	}
	return list;
}

void KileProject::setMasterDocument(const QString & master){
	
	if(!master.isEmpty()){
	
		QFileInfo fi(master);
		if(fi.exists())
			m_masterDocument = master;
		else{
			m_masterDocument = QString::null;
			KILE_DEBUG() << "setMasterDocument: masterDoc=NULL";	
		}
	
	}
	else
		m_masterDocument = QString::null;
	
	emit (masterDocumentChanged(m_masterDocument));
}

#include "kileproject.moc"