
#include "make_preview.hpp"

#include "../common/engine.hpp"

#include "../common/lib/sqlite/sqlite3.h"

template<typename RES_T>
std::shared_ptr<RES_T> loadFS(const std::string& path) {
	DataSourceFilesystem ds(path);
	auto strm = ds.make_stream();
	std::shared_ptr<RES_T> res(new RES_T);
	res->deserialize(*strm.get(), strm->bytes_available());
	return res;
}

sqlite3* _db;

bool dbInit() {
	std::string path = MKSTR(get_module_dir() << "/meta.db");
	int rc = sqlite3_open_v2(path.c_str(), &_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
    if(rc) {
        LOG_WARN("Failed to open meta database: " << sqlite3_errmsg(_db));
        return false;
    }
    sqlite3_exec(_db, "CREATE TABLE IF NOT EXISTS thumbnails (resource_id TEXT PRIMARY KEY, timestamp INTEGER, state INTEGER, png BLOB)", 0, 0, 0);
    return true;
}

void dbCleanup() {
	sqlite3_close_v2(_db);
}

bool dbInsertOrReplace(const std::string& res_path, int64_t timestamp, int32_t state, std::shared_ptr<Texture2D> tex) {
	sqlite3_stmt* stmt = 0;
	int rc = sqlite3_prepare_v2(_db, "INSERT OR REPLACE INTO thumbnails(resource_id, timestamp, state, png) VALUES(?, ?, ?, ?)", -1, &stmt, NULL);
	if(rc) {
		LOG_WARN("sqlite3_prepare_v2 failed: " << sqlite3_errmsg(_db));
		return false;
	}
	rc = sqlite3_bind_text(stmt, 1, res_path.data(), res_path.size(), 0);
	if(rc) {
		LOG_WARN("sqlite3_bind_text failed: " << sqlite3_errmsg(_db));
		return false;
	}
	rc = sqlite3_bind_int64(stmt, 2, timestamp);
	if(rc) {
		LOG_WARN("sqlite3_bind_int64 failed: " << sqlite3_errmsg(_db));
		return false;
	}
	rc = sqlite3_bind_int(stmt, 3, state);
	if(rc) {
		LOG_WARN("sqlite3_bind_int failed: " << sqlite3_errmsg(_db));
		return false;
	}
	if(tex) {
		dstream data_strm;
		tex->serialize(data_strm);
		std::vector<char> data_buf = data_strm.getBuffer();
		rc = sqlite3_bind_blob(stmt, 4, data_buf.data(), data_buf.size(), 0);
	} else {
		rc = sqlite3_bind_blob(stmt, 4, 0, 0, 0);
	}
	if(rc) {
		LOG_WARN("sqlite3_bind_blob failed: " << sqlite3_errmsg(_db));
		return false;
	}
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	return true;
}

bool dbPrepareRecord(const std::string& res_path) {
	sqlite3_stmt* stmt = 0;
	int rc = sqlite3_prepare_v2(_db, "INSERT OR REPLACE INTO thumbnails(resource_id, timestamp, state, png) VALUES(?, ?, ?, ?)", -1, &stmt, NULL);
	if(rc) {
		LOG_WARN("sqlite3_prepare_v2 failed: " << sqlite3_errmsg(_db));
		return false;
	}
	rc = sqlite3_bind_text(stmt, 1, res_path.data(), res_path.size(), 0);
	if(rc) {
		LOG_WARN("sqlite3_bind_text failed: " << sqlite3_errmsg(_db));
		return false;
	}
	rc = sqlite3_bind_int64(stmt, 2, time(0) /* TODO: store proper file modify timestamp */);
	if(rc) {
		LOG_WARN("sqlite3_bind_int64 failed: " << sqlite3_errmsg(_db));
		return false;
	}
	rc = sqlite3_bind_int(stmt, 3, 1 /* 1 - Invalid. The record needs to be invalidated before rendering a preview */);
	if(rc) {
		LOG_WARN("sqlite3_bind_int failed: " << sqlite3_errmsg(_db));
		return false;
	}
	rc = sqlite3_bind_blob(stmt, 4, 0, 0, 0);
	if(rc) {
		LOG_WARN("sqlite3_bind_blob failed: " << sqlite3_errmsg(_db));
		return false;
	}
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	return true;
}

bool dbStorePreview(const std::string& res_path, std::shared_ptr<Texture2D> texture) {
	sqlite3_stmt* stmt = 0;
	int rc = sqlite3_prepare_v2(_db, "INSERT OR REPLACE INTO thumbnails(resource_id, timestamp, state, png) VALUES(?, ?, ?, ?)", -1, &stmt, NULL);
	if(rc) {
		LOG_WARN("sqlite3_prepare_v2 failed: " << sqlite3_errmsg(_db));
		return false;
	}
	rc = sqlite3_bind_text(stmt, 1, res_path.data(), res_path.size(), 0);
	if(rc) {
		LOG_WARN("sqlite3_bind_text failed: " << sqlite3_errmsg(_db));
		return false;
	}
	rc = sqlite3_bind_int64(stmt, 2, time(0) /* TODO: store proper file modify timestamp */);
	if(rc) {
		LOG_WARN("sqlite3_bind_int64 failed: " << sqlite3_errmsg(_db));
		return false;
	}
	rc = sqlite3_bind_int(stmt, 3, 0 /* 0 - OK */);
	if(rc) {
		LOG_WARN("sqlite3_bind_int failed: " << sqlite3_errmsg(_db));
		return false;
	}
	dstream data_strm;
	texture->serialize(data_strm);
	std::vector<char> data_buf = data_strm.getBuffer();
	rc = sqlite3_bind_blob(stmt, 4, data_buf.data(), data_buf.size(), 0);
	if(rc) {
		LOG_WARN("sqlite3_bind_blob failed: " << sqlite3_errmsg(_db));
		return false;
	}
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	return true;
}

#include <windows.h>

int main(int argc, char** argv) {
	SetErrorMode(SetErrorMode(0) | SEM_NOGPFAULTERRORBOX);

	if(argc < 2) {
		return 1;
	}
	std::string res_path = argv[1];

	if(!dbInit()) {
		return 1;
	}

	PlatformStartupParams params = { 0 };
	params.hide_window = true;
	if(!katanaInit(&params)) {
        LOG_ERR("Failed to initialize platform");
		dbCleanup();
        return 1;
    }
	ResourceDescLibrary::get()->init();

	dbInsertOrReplace(
		res_path, 
		time(0), // TODO: Use actual file modify time
		2, // Unknown
		0
	);

	rttr::type type = ResourceDescLibrary::get()->findType(res_path);
	std::shared_ptr<Texture2D> texture;
	if(type == rttr::type::get<Texture2D>()) {
		dbInsertOrReplace(
			res_path,
			time(0), // TODO: Use actual file modify time
			1,
			0
		); // Set as invalid
		texture = makePreview(retrieve<Texture2D>(res_path));
	} else if(type == rttr::type::get<GameScene>()) {
		dbInsertOrReplace(
			res_path,
			time(0), // TODO: Use actual file modify time
			1,
			0
		); // Set as invalid
		texture = makePreview(retrieve<GameScene>(res_path));
	} else if(type == rttr::type::get<ModelSource>()) {
		dbInsertOrReplace(
			res_path,
			time(0), // TODO: Use actual file modify time
			1,
			0
		); // Set as invalid
		texture = makePreview(retrieve<ModelSource>(res_path));
	} else if(type == rttr::type::get<Material>()) {
		dbInsertOrReplace(
			res_path,
			time(0), // TODO: Use actual file modify time
			1,
			0
		); // Set as invalid
		texture = makePreview(retrieve<Material>(res_path));
	}

	if(texture) {
		/*
		dbInsertOrReplace(
			res_path,
			time(0), // TODO: Use actual file modify time
			0, // OK
			texture
		);
		*/
		dbStorePreview(res_path, texture);
	} else {
		LOG_ERR("Unsupported resource: " << res_path);
		dbCleanup();
		katanaCleanup();
		return 1;
	}

	dbCleanup();
	katanaCleanup();
	return 0;
}