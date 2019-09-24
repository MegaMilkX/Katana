
#include "make_preview.hpp"

#include "../common/platform/platform.hpp"
#include "../common/util/log.hpp"

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
    sqlite3_exec(_db, "CREATE TABLE IF NOT EXISTS thumbnails (resource_id TEXT PRIMARY KEY, png BLOB)", 0, 0, 0);
    return true;
}

void dbCleanup() {
	sqlite3_close_v2(_db);
}

bool dbStorePreview(const std::string& res_path, std::shared_ptr<Texture2D> texture) {
	sqlite3_stmt* stmt = 0;
	int rc = sqlite3_prepare_v2(_db, "INSERT OR REPLACE INTO thumbnails(resource_id, png) VALUES(?, ?)", -1, &stmt, NULL);
	if(rc) {
		LOG_WARN("sqlite3_prepare_v2 failed: " << sqlite3_errmsg(_db));
		return false;
	}
	rc = sqlite3_bind_text(stmt, 1, res_path.data(), res_path.size(), 0);
	if(rc) {
		LOG_WARN("sqlite3_bind_text failed: " << sqlite3_errmsg(_db));
		return false;
	}
	dstream data_strm;
	texture->serialize(data_strm);
	std::vector<char> data_buf = data_strm.getBuffer();
	rc = sqlite3_bind_blob(stmt, 2, data_buf.data(), data_buf.size(), 0);
	if(rc) {
		LOG_WARN("sqlite3_bind_blob failed: " << sqlite3_errmsg(_db));
		return false;
	}
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	return true;
}

int main(int argc, char** argv) {
	if(argc < 2) {
		return 1;
	}
	std::string res_path = argv[1];

	if(!dbInit()) {
		return 1;
	}

	PlatformStartupParams params;
	params.hide_window = true;
	if(!platformInit(&params)) {
        LOG_ERR("Failed to initialize platform");
		dbCleanup();
        return 1;
    }
	ResourceDescLibrary::get()->init();

	rttr::type type = ResourceDescLibrary::get()->findType(res_path);
	std::shared_ptr<Texture2D> texture;
	if(type == rttr::type::get<Texture2D>()) {
		texture = makePreview(retrieve<Texture2D>(res_path));
	} else if(type == rttr::type::get<GameScene>()) {
		texture = makePreview(retrieve<GameScene>(res_path));
	} else if(type == rttr::type::get<ModelSource>()) {
		texture = makePreview(retrieve<ModelSource>(res_path));
	} else if(type == rttr::type::get<Material>()) {
		texture = makePreview(retrieve<Material>(res_path));
	}

	if(texture) {
		dbStorePreview(res_path, texture);
	} else {
		LOG_ERR("Unsupported resource: " << res_path);
		dbCleanup();
		platformCleanup();
		return 1;
	}

	dbCleanup();
	platformCleanup();
	return 0;
}