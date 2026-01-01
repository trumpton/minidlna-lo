/* MiniDLNA media server
 * Copyright (C) 2008-2017  Justin Maggard
 *
 * This file is part of MiniDLNA.
 *
 * MiniDLNA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * MiniDLNA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MiniDLNA. If not, see <http://www.gnu.org/licenses/>.
 */

#define MAXIDLEN 128

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <locale.h>
#include <libgen.h>
#include <inttypes.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "config.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#endif
#include <sqlite3.h>
#include "libav.h"

#include "scanner_sqlite.h"
#include "upnpglobalvars.h"
#include "metadata.h"
#include "playlist.h"
#include "utils.h"
#include "sql.h"
#include "scanner.h"
#include "albumart.h"
#include "containers.h"
#include "log.h"
#include "monitor.h"

#if SCANDIR_CONST
typedef const struct dirent scan_filter;
#else
typedef struct dirent scan_filter;
#endif
#ifndef AV_LOG_PANIC
#define AV_LOG_PANIC AV_LOG_FATAL
#endif

// Used by monitor.c
int valid_cache = 0;

//
// AddPlaylistToDatabase
//
// Description:
//   Adds playlist to the PLAYLISTS database table
//
// Parameters:
//   TO BE COMPLETED
//
// Returns:
//   Nothing
//
void
AddPlaylistToDatabase(int searchhandle, const char *path, const char *name)
{
	insert_playlist(path, name) ;
}

//
// AddFileToDatabase
//
// Description:
//   Given a layout search handle (for a chain). and details of the file
//   to be added, adds appropriate entries into the OBJECTS database table.
//
// Parameters:
//   int searchhandle      - Handle to use with global layout object to obtain
//                           details of the chain.
//   int64_t mediaDetailID - ID of the object in the DETAILS table to add to
//                           the OBJECTS table.
// Returns:
//   Nothing
//
void
AddFileToDatabase(int searchhandle, int64_t mediaDetailID)
{
	char *path = GetMetadataPathFromId(mediaDetailID) ;
	char parentID[MAXIDLEN], objectID[MAXIDLEN], mediaRefID[MAXIDLEN] ;
	const char *rootID = "0" ;
	strcpy(mediaRefID, "") ;

	// Step through the chains
	int numchains = layout_numchains(lo, searchhandle) ;
	for (int chainnum=0; chainnum<numchains; chainnum++) {

		DPRINTF(E_DEBUG, L_SCANNER, "layout processing chain=%d (%s)\n",
				chainnum, layout_chainitems(lo, searchhandle, chainnum)) ;

		// Initialise objectID and parentID
		strcpy(objectID, "") ;
		strcpy(parentID, rootID) ;

		char *fieldname, *fieldclass ;
		int fieldnum=0 ;

		// Step through the fields in a chain
		while (layout_findfield(lo, searchhandle, chainnum, fieldnum, &fieldname, &fieldclass)) {

			// Determine whether the current field is a container or an item
			int iscontainer = fieldclass && (strncmp(fieldclass, "container.", 10)==0) ;
			int isalbum = fieldclass && (strncmp(fieldclass, "container.album", 15)==0) ;

			// Search for the entry in the OBJECTS table
			char *escapedfieldname = layout_escape(fieldname) ;

			char *ret = sql_get_text_field(db,
				"SELECT OBJECT_ID from OBJECTS where PARENT_ID = '%s' and NAME = '%s' and CLASS = '%s';",
				parentID, escapedfieldname, fieldclass) ;

			DPRINTF(E_DEBUG, L_SCANNER,
					"SELECT OBJECT_ID from OBJECTS where PARENT_ID = '%s' and NAME = '%s' and CLASS = '%s'; => %s\n",
				parentID, escapedfieldname, fieldclass, ret?ret:"NULL") ;

			free(escapedfieldname) ;

			if (ret) {

				// Found in the OBJECTS table, so no need to add anything apart from
				// updating the objectID ready for the next loop
				DPRINTF(E_DEBUG, L_SCANNER, "Select found match => %s\n", ret) ;
				strncpy(objectID, ret, sizeof(objectID)-1) ;
				sqlite3_free(ret) ;

			} else {

				// Not found
				int64_t detailID ;

				if (iscontainer) {

					// If the item is an album container, add to the DETAILS table
					DPRINTF(E_DEBUG, L_SCANNER, "  XX folder => %s, %s, %s\n",
							fieldname, path, isalbum?"true":"false") ;
					detailID = FindFolderMetadata(fieldname, path, isalbum) ;

				} else {

					// Otherwise, the item is a media file, use the supplied detailID
					DPRINTF(E_DEBUG, L_SCANNER, "  XX mediafile => %s, %s, %ld\n",
							fieldname, path, find_album_art(path, NULL, 0)) ;
					detailID = mediaDetailID ;
				}

				// Containers don't have a refID
				// the mediaRefID starts off NULL (for the first media entry)
				// and thereafter refers to the first media entry
				const char *refID = iscontainer ? NULL : mediaRefID ;

				// Find the next available entry and build the objectID
				int64_t nextID = get_next_available_id("OBJECTS", parentID) ;

				if (strcmp(parentID,rootID)==0) {

					// Root item, so don't include the parentID in the objectID
					// strcpy(parentID, rootID) ;
					if (nextID==0) {
						// This is the very first entry in the database (aside from the root)
						// So force the nextID to 0x100.
						nextID = 256 ;
					}
					snprintf(objectID, sizeof(objectID), "%lX", nextID) ;

				} else {

					// child item, so copy objectID to parentID and build new objectID
					strcpy(parentID, objectID) ;
					snprintf(objectID, sizeof(objectID), "%s$%lX", parentID, nextID) ;

				}

				// Now add to the OBJECTS table

				DPRINTF(E_DEBUG, L_SCANNER,
						"INSERT into OBJECTS (REF_ID, OBJECT_ID, PARENT_ID, DETAIL_ID, NAME, CLASS) "
						"VALUES"
						" ('%s', '%s', '%s', %lld, '%s', '%s');",
						iscontainer?"NULL":refID, objectID, parentID, (long long)detailID,
						fieldname, fieldclass) ;

				sql_exec(db,
						"INSERT into OBJECTS (REF_ID, OBJECT_ID, PARENT_ID, DETAIL_ID, NAME, CLASS) "
						"VALUES"
						" ('%q', '%q', '%q', %lld, '%q', '%q');",
						iscontainer?NULL:refID, objectID, parentID, (long long)detailID,
						fieldname, fieldclass) ;

				// Save mediaRefID as reference to first instance of objectID
				if (!iscontainer && mediaRefID[0]=='\0') {
					strcpy(mediaRefID, objectID) ;
				}

			}

			// Ready the parentID for the next part of the path within the chain
			fieldnum++ ;

			// Update the parentID to be the current objectID
			strcpy(parentID, objectID) ;
		}
	}
	free(path) ;
}

//
// Function:
//   get_next_available_id
//
// Description:
//   Finds the next available ID at this level of hierarchy within the specified
//   database table.
//
// Parameters:
//   char *table     - Database table to use - e.g. OBJECTS
//   char *parentID  - ID of the parent entry in the table - e.g. 20$34$12
//
// Returns:
//   int64_t n       - Next available entry, so objectID would be "20$34$12$" n
//
int64_t
get_next_available_id(const char *table, const char *parentID)
{
		char *ret, *base;
		int64_t objectID = 0;

		ret = sql_get_text_field(db, "SELECT OBJECT_ID from %s where ID = "
		                             "(SELECT max(ID) from %s where PARENT_ID = '%s')",
		                             table, table, parentID);
		if( ret )
		{
			base = strrchr(ret, '$');
			if( base )
				objectID = strtoll(base+1, NULL, 16) + 1;
			else // added ELSE statement for LAYOUT support
				objectID = strtoll(ret, NULL, 16) + 1;
			sqlite3_free(ret);
		}

		return objectID;
}



//
// Function:
//   insert_file
//
// Description:
//   Inserts a media file into the database.  This involves calling one of the GetXxxMetadata
//   functions which extracts metadata and populates a new entry in the DETAILS database table
//   and then searching the layout configuration for a matching set of chains, and finally,
//   inserting the file or the playlist to the database.
//
// Parameters:
//   char *name      - Name of the file to be added
//   char *path      - Full pathname to the file to be added
//
int
insert_file(const char *name, const char *path)
{
	int64_t detailID = 0;
	media_types mtype = get_media_type(name);

	// Based on the media type, add the details to the DETAILS
	// table and obtain the detailID.
	if (mtype & TYPE_IMAGE) {
		if (is_album_art(path)) return -1 ;
		detailID = GetImageMetadata(path, name);
	} else if (mtype & TYPE_AUDIO) {
		detailID = GetAudioMetadata(path, name);
	} else if (mtype & TYPE_VIDEO) {
		detailID = GetVideoMetadata(path, name);
		if (!detailID) {
			// If failed to extract video Metadata, assume the item is an audio file
			mtype=TYPE_AUDIO ;
			detailID = GetAudioMetadata(path, name) ;
		}
	} else if (mtype & TYPE_PLAYLIST) {
		// No processing required ??? TBC TODO
	}

	// If unable to extract details, abort - file is not a media file
	if (!detailID && !(mtype & TYPE_PLAYLIST)) {
		DPRINTF(E_WARN, L_SCANNER, "Unsuccessful getting details for %s\n", path);
		return -1 ;
	}

	// Log insertion of file into database
	DPRINTF(E_DEBUG, L_SCANNER, "insert_file(%s) => %s\n", path, mtype&TYPE_IMAGE?"image":
																mtype&TYPE_AUDIO?"audio":
																mtype&TYPE_VIDEO?"video":
																mtype&TYPE_PLAYLIST?"playlist":
																"unknown") ;

	// Get Media File Details
	metadata_t *m =	GetMetadataDetailsFromId(detailID) ;
	char * mediatype = mtype & TYPE_VIDEO ? "video" :
					   mtype & TYPE_AUDIO ? "audio" :
					   mtype & TYPE_IMAGE ? "image" :
					   mtype & TYPE_PLAYLIST ? "playlist" :
					   "unknown" ;

	// Find the appropriate chain(s)
	int searchhandle = layout_search(lo, mediatype, path, m) ;

	// Add entry into OBJECTS table in the database using the layout chains
	if (detailID) {
		AddFileToDatabase(searchhandle, detailID) ;
	} else {
		AddPlaylistToDatabase(searchhandle, path, name) ;
	}

	// Tidy up
	metadata_tidy_free(m) ;

	return 0;
}

//
// Function:
//   CreateDatabase
//
// Description:
//   Creates an empty database, and adds a root starting entry
//
int
CreateDatabase(void)
{
	int ret ;

	ret = sql_exec(db, create_objectTable_sqlite);
	if( ret != SQLITE_OK )
		goto sql_failed;
	ret = sql_exec(db, create_detailTable_sqlite);
	if( ret != SQLITE_OK )
		goto sql_failed;
	ret = sql_exec(db, create_albumArtTable_sqlite);
	if( ret != SQLITE_OK )
		goto sql_failed;
	ret = sql_exec(db, create_captionTable_sqlite);
	if( ret != SQLITE_OK )
		goto sql_failed;
	ret = sql_exec(db, create_bookmarkTable_sqlite);
	if( ret != SQLITE_OK )
		goto sql_failed;
	ret = sql_exec(db, create_playlistTable_sqlite);
	if( ret != SQLITE_OK )
		goto sql_failed;
	ret = sql_exec(db, create_settingsTable_sqlite);
	if( ret != SQLITE_OK )
		goto sql_failed;
	ret = sql_exec(db, "INSERT into SETTINGS values ('UPDATE_ID', '0')");
	if( ret != SQLITE_OK )
		goto sql_failed;

	// Create ROOT Entry in OBJECTS
	ret = sql_exec(db, "INSERT into OBJECTS (OBJECT_ID, PARENT_ID, DETAIL_ID, CLASS, NAME)"
                   " values ('0', '-1', 0, 'container.storageFolder', 'root');");
	if( ret != SQLITE_OK )
		goto sql_failed;

	// Create ROOT Entry in DETAILS
	ret = sql_exec(db, "INSERT into DETAILS (TITLE) values ('root');");
	if( ret != SQLITE_OK )
		goto sql_failed;

	sql_exec(db, "create INDEX IDX_OBJECTS_OBJECT_ID ON OBJECTS(OBJECT_ID);");
	sql_exec(db, "create INDEX IDX_OBJECTS_PARENT_ID ON OBJECTS(PARENT_ID);");
	sql_exec(db, "create INDEX IDX_OBJECTS_DETAIL_ID ON OBJECTS(DETAIL_ID);");
	sql_exec(db, "create INDEX IDX_OBJECTS_CLASS ON OBJECTS(CLASS);");
	sql_exec(db, "create INDEX IDX_DETAILS_PATH ON DETAILS(PATH);");
	sql_exec(db, "create INDEX IDX_DETAILS_ID ON DETAILS(ID);");
	sql_exec(db, "create INDEX IDX_ALBUM_ART ON ALBUM_ART(ID);");
	sql_exec(db, "create INDEX IDX_SCANNER_OPT ON OBJECTS(PARENT_ID, NAME, OBJECT_ID);");

sql_failed:
	if( ret != SQLITE_OK )
		DPRINTF(E_ERROR, L_DB_SQL, "Error creating SQLite3 database!\n");
	return (ret != SQLITE_OK);
}


//
// Function:
//   filter_hidden, filter_type, filter_avp
//
// Description:
//   Returns true if file is a wanted media file.  filter_avp is used by the
//   scandir function within ScanDirectory.
//
static inline int
filter_hidden(scan_filter *d)
{
	return (d->d_name[0] != '.');
}

static int
filter_type(scan_filter *d)
{
#if HAVE_STRUCT_DIRENT_D_TYPE
	return ( (d->d_type == DT_DIR) ||
		 (d->d_type == DT_LNK) ||
		 (d->d_type == DT_UNKNOWN)
		);
#else
	return 1;
#endif
}

static int
filter_avp(scan_filter *d)
{
	return ( filter_hidden(d) &&
		 (filter_type(d) ||
		  (is_reg(d) &&
		   (is_audio(d->d_name) ||
		    is_image(d->d_name) ||
		    is_video(d->d_name) ||
		    is_playlist(d->d_name))))
		);
}

//
// Function:
//   ScanDirectory
//
// Description:
//   Herarchically scanns the given directory, and adds files to the database
//   UP TO HERE !!!
//
static void
ScanDirectory(const char *dir, int isroot)
{
	struct dirent **namelist;
	int n;
	char *full_path;
	char *name = NULL;
	static long long unsigned int fileno = 0;
	enum file_types type;

	DPRINTF(isroot?E_INFO:E_WARN, L_SCANNER, _("Scanning %s\n"), dir);
	n = scandir(dir, &namelist, filter_avp, alphasort);

	if( n < 0 )
	{
		DPRINTF(E_WARN, L_SCANNER, "Error scanning %s [%s]\n",
			dir, strerror(errno));
		return;
	}

	full_path = malloc(PATH_MAX);
	if (!full_path)
	{
		DPRINTF(E_ERROR, L_SCANNER, "Memory allocation failed scanning %s\n", dir);
		return;
	}

	for (int i=0; i < n; i++)
	{
#if !USE_FORK
		if( quitting )
			break;
#endif
		type = TYPE_UNKNOWN;
		snprintf(full_path, PATH_MAX, "%s/%s", dir, namelist[i]->d_name);
		name = escape_tag(namelist[i]->d_name, 1);
		if( is_dir(namelist[i]) == 1 )
		{
			type = TYPE_DIR;
		}
		else if( is_reg(namelist[i]) == 1 )
		{
			type = TYPE_FILE;
		}
		else
		{
			type = resolve_unknown_type(full_path);
		}

		if( (type == TYPE_DIR) && (access(full_path, R_OK|X_OK) == 0) )
		{
			ScanDirectory(full_path, 0);
		}

		if( type == TYPE_FILE && (access(full_path, R_OK) == 0) )
		{
			if( insert_file(name, full_path) == 0 )
				fileno++;
		}
		free(name);
		free(namelist[i]);
	}
	free(namelist);
	free(full_path);
	if( isroot )
	{
		DPRINTF(E_WARN, L_SCANNER, _("Scanning %s finished (%llu files)!\n"), dir, fileno);
	}
}


/* rescan functions added by shrimpkin@sourceforge.net */
static int
cb_orphans(void *args, int argc, char **argv, char **azColName)
{
	const char *path = argv[0];
	const char *mime = argv[1];

	/* If we can't access the path, remove it */
	if (access(path, R_OK) != 0)
	{
		DPRINTF(E_DEBUG, L_SCANNER, "Removing %s [%s]\n", path, mime ? "file" : "dir");
		if (mime)
			monitor_remove_file(path);
		else
			monitor_remove_directory(0, path);
	}

	return 0;
}

void
start_rescan(void)
{
	struct media_dir_s *media_path;
	char *esc_name = NULL;
	char *zErrMsg;
	const char *sql_files = "SELECT path, mime FROM details WHERE path NOT NULL AND mime IS NOT NULL;";
	const char *sql_dir = "SELECT path, mime FROM details WHERE path NOT NULL AND mime IS NULL;";
	int changes = sqlite3_total_changes(db);
	const char *summary;
	int ret;

	DPRINTF(E_INFO, L_SCANNER, "Starting rescan\n");

	/* Find and remove any dead directory links */
	ret = sqlite3_exec(db, sql_dir, cb_orphans, NULL, &zErrMsg);
	if (ret != SQLITE_OK)
	{
		DPRINTF(E_MAXDEBUG, L_SCANNER, "SQL error: %s\nBAD SQL: %s\n", zErrMsg, sql_dir);
		sqlite3_free(zErrMsg);
	}

	/* Find and remove any dead file links */
	ret = sqlite3_exec(db, sql_files, cb_orphans, NULL, &zErrMsg);
	if (ret != SQLITE_OK)
	{
		DPRINTF(E_MAXDEBUG, L_SCANNER, "SQL error: %s\nBAD SQL: %s\n", zErrMsg, sql_files);
		sqlite3_free(zErrMsg);
	}

	/* Rescan media_paths for new and/or modified files */
	for (media_path = media_dirs; media_path != NULL; media_path = media_path->next)
	{
		char path[MAXPATHLEN], buf[MAXPATHLEN];
		strncpyt(path, media_path->path, sizeof(path));
		strncpyt(buf, media_path->path, sizeof(buf));
		esc_name = escape_tag(basename(buf), 1);
		monitor_insert_directory(0, esc_name, path);
		free(esc_name);
	}
	fill_playlists();

	if (sqlite3_total_changes(db) != changes)
		summary = "changes found";
	else
		summary = "no changes";
	DPRINTF(E_INFO, L_SCANNER, "Rescan completed. (%s)\n", summary);
}
/* end rescan functions */



void
start_scanner(void)
{
	struct media_dir_s *media_path;

	if (setpriority(PRIO_PROCESS, 0, 15) == -1)
		DPRINTF(E_WARN, L_INOTIFY,  "Failed to reduce scanner thread priority\n");

	setlocale(LC_COLLATE, "");
	lav_register_all();
	av_log_set_level(AV_LOG_PANIC);

	if( GETFLAG(RESCAN_MASK) )
		return start_rescan();

	for( media_path = media_dirs; media_path != NULL; media_path = media_path->next )
	{
		ScanDirectory(media_path->path, 1);
		sql_exec(db, "INSERT into SETTINGS values (%Q, %Q)", "media_dir", media_path->path);
	}
	/* Create this index after scanning, so it doesn't slow down the scanning process.
	 * This index is very useful for large libraries used with an XBox360 (or any
	 * client that uses UPnPSearch on large containers). */
	sql_exec(db, "create INDEX IDX_SEARCH_OPT ON OBJECTS(OBJECT_ID, CLASS, DETAIL_ID);");

	fill_playlists();

	DPRINTF(E_DEBUG, L_SCANNER, "Initial file scan completed\n");
	//JM: Set up a db version number, so we know if we need to rebuild due to a new structure.
	sql_exec(db, "pragma user_version = %d;", DB_VERSION);
}
