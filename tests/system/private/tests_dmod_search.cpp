#define DMOD_PRIVATE
#include "dmod.h"
#include "private/dmod_hlp.h"
#include <gtest/gtest.h>
#include <atomic>
#include <algorithm>
#include <dirent.h>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

static std::atomic<unsigned> opens{0};
extern "C" void* Dmod_OpenDir(const char* path)
{
    ++opens;
    void* dir = opendir(path);
    return dir;
}

using Nodes = std::unique_ptr<Dmod_SearchNode_t, decltype(&Dmod_Hlp_FreeSearchPathList)>;
static Nodes Snapshot() { return Nodes(Dmod_Hlp_PrepareModulesSearchNodes(), Dmod_Hlp_FreeSearchPathList); }
static std::vector<std::string> Paths(Dmod_SearchNode_t* node)
{
    std::vector<std::string> paths;
    for (; node; node = node->Prev) paths.emplace_back(node->Path);
    return paths;
}
static bool Contains(Dmod_SearchNode_t* node, const std::string& path)
{
    for (; node; node = node->Prev) if (path == node->Path) return true;
    return false;
}

class SearchCacheTest : public ::testing::Test
{
protected:
    std::string root, oldRepo;
    bool hadRepo;
    std::vector<std::string> dirs, files;
    void SetUp() override
    {
        const char* old = getenv("DMOD_REPO_PATHS");
        hadRepo = old != nullptr;
        oldRepo = old ? old : "";
        char temp[] = "/tmp/dmod-search-XXXXXX";
        ASSERT_NE(mkdtemp(temp), nullptr);
        root = temp;
        setenv("DMOD_REPO_PATHS", (root + "/*/bin").c_str(), 1);
        Dmod_Hlp_InitSearchCache();
        Dmod_InvalidateModuleSearchPaths();
        opens = 0;
    }
    void TearDown() override
    {
        Dmod_Hlp_DeinitSearchCache();
        if (hadRepo) setenv("DMOD_REPO_PATHS", oldRepo.c_str(), 1);
        else unsetenv("DMOD_REPO_PATHS");
        for (auto& file : files) unlink(file.c_str());
        for (auto it = dirs.rbegin(); it != dirs.rend(); ++it) rmdir(it->c_str());
        rmdir(root.c_str());
    }
    void Directory(const std::string& name)
    {
        std::string path = root + "/" + name;
        ASSERT_EQ(mkdir(path.c_str(), 0700), 0);
        dirs.push_back(path);
    }
};

TEST_F(SearchCacheTest, ReusesSnapshotWithoutDirectoryReads)
{
    Directory("CARD");
    auto first = Snapshot();
    ASSERT_TRUE(Contains(first.get(), root + "/CARD/bin"));
    unsigned before = opens;
    for (int i = 0; i < 100; ++i)
    {
        auto next = Snapshot();
        EXPECT_EQ(next.get(), first.get());
    }
    EXPECT_EQ(opens, before);
}

TEST_F(SearchCacheTest, MountAndUnmountRequireInvalidationAndPreserveReaders)
{
    auto empty = Snapshot();
    Directory("CARD");
    auto cached = Snapshot();
    EXPECT_FALSE(Contains(cached.get(), root + "/CARD/bin"));
    Dmod_InvalidateModuleSearchPaths();
    auto mounted = Snapshot();
    ASSERT_TRUE(Contains(mounted.get(), root + "/CARD/bin"));
    ASSERT_EQ(rmdir((root + "/CARD").c_str()), 0);
    Dmod_InvalidateModuleSearchPaths();
    auto unmounted = Snapshot();
    EXPECT_FALSE(Contains(unmounted.get(), root + "/CARD/bin"));
    EXPECT_TRUE(Contains(mounted.get(), root + "/CARD/bin"));
}

TEST_F(SearchCacheTest, NewBinAndFilesAreVisibleWithoutInvalidation)
{
    Directory("CARD");
    auto before = Snapshot();
    ASSERT_TRUE(Contains(before.get(), root + "/CARD/bin"));
    Directory("CARD/bin");
    for (const auto& ext : {".dmf", ".dmfc"})
    {
        std::string file = root + "/CARD/bin/fresh" + ext;
        files.push_back(file);
        std::ofstream(file) << "uploaded";
        EXPECT_TRUE(Dmod_FileAvailable(file.c_str()));
    }
    unsigned count = opens;
    auto after = Snapshot();
    EXPECT_EQ(after.get(), before.get());
    EXPECT_EQ(opens, count);
}

TEST_F(SearchCacheTest, ExpandsCardsAndSkipsFilesAndDotEntries)
{
    Directory("Z"); Directory("A");
    files.push_back(root + "/regular-file");
    std::ofstream(files.back()) << "data";
    auto snapshot = Snapshot();
    std::vector<std::string> cards;
    for (const auto& p : Paths(snapshot.get())) if (p.find(root + "/") == 0) cards.push_back(p);
    std::sort(cards.begin(), cards.end());
    EXPECT_EQ(cards, (std::vector<std::string>{root + "/A/bin", root + "/Z/bin"}));
}

TEST_F(SearchCacheTest, ConfigurationChangesRequireInvalidation)
{
    Directory("A");
    auto before = Snapshot();
    setenv("DMOD_REPO_PATHS", "/other/context", 1);
    auto cached = Snapshot();
    EXPECT_EQ(cached.get(), before.get());
    EXPECT_FALSE(Contains(cached.get(), "/other/context"));
    Dmod_InvalidateModuleSearchPaths();
    auto after = Snapshot();
    EXPECT_TRUE(Contains(after.get(), "/other/context"));
    EXPECT_FALSE(Contains(after.get(), root + "/A/bin"));
    EXPECT_TRUE(Contains(before.get(), root + "/A/bin"));
}

TEST_F(SearchCacheTest, PreservesLiteralPrecedenceAndSupportsWildcardWithoutSuffix)
{
    Directory("A");
    setenv("DMOD_REPO_PATHS", (root + "/*:/first:/last").c_str(), 1);
    auto snapshot = Snapshot();
    auto paths = Paths(snapshot.get());
    auto first = std::find(paths.begin(), paths.end(), "/first");
    auto last = std::find(paths.begin(), paths.end(), "/last");
    auto card = std::find(paths.begin(), paths.end(), root + "/A");
    EXPECT_LT(last, first);
    EXPECT_LT(first, card);
}

TEST_F(SearchCacheTest, RejectsUnsupportedWildcards)
{
    for (const char* pattern : {"relative/*/bin", "/mnt/card*/bin", "/mnt/*/*/bin"})
    {
        setenv("DMOD_REPO_PATHS", pattern, 1);
        Dmod_InvalidateModuleSearchPaths();
        auto snapshot = Snapshot();
        EXPECT_FALSE(Contains(snapshot.get(), pattern));
    }
}

TEST_F(SearchCacheTest, ConcurrentReadersAndInvalidation)
{
    Directory("CARD");
    std::atomic<bool> bad{false};
    std::vector<std::thread> readers;
    for (int t = 0; t < 4; ++t) readers.emplace_back([&] {
        for (int i = 0; i < 500; ++i)
        {
            auto snapshot = Snapshot();
            if (!Contains(snapshot.get(), root + "/CARD/bin")) bad = true;
            std::this_thread::yield();
            if (!Contains(snapshot.get(), root + "/CARD/bin")) bad = true;
        }
    });
    for (int i = 0; i < 500; ++i) { Dmod_InvalidateModuleSearchPaths(); std::this_thread::yield(); }
    for (auto& reader : readers) reader.join();
    EXPECT_FALSE(bad);
}

TEST_F(SearchCacheTest, LookupsBeforeInitializationRemainUncached)
{
    Dmod_Hlp_DeinitSearchCache();
    auto before = Snapshot();
    Directory("CARD");
    auto after = Snapshot();
    ASSERT_TRUE(Contains(after.get(), root + "/CARD/bin"));
    EXPECT_FALSE(Contains(before.get(), root + "/CARD/bin"));
    ASSERT_NE(after.get(), nullptr);
    EXPECT_EQ(after->References, 0u);
    Dmod_Hlp_InitSearchCache();
    auto cached = Snapshot();
    auto reused = Snapshot();
    EXPECT_EQ(cached.get(), reused.get());
}

TEST_F(SearchCacheTest, ReinitializationDropsOldConfigurationCache)
{
    Directory("A");
    {
        auto before = Snapshot();
        ASSERT_TRUE(Contains(before.get(), root + "/A/bin"));
    }
    Dmod_Hlp_DeinitSearchCache();
    Directory("B");
    Dmod_Hlp_InitSearchCache();
    auto after = Snapshot();
    EXPECT_TRUE(Contains(after.get(), root + "/A/bin"));
    EXPECT_TRUE(Contains(after.get(), root + "/B/bin"));
}
