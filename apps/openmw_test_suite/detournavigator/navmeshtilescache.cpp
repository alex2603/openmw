#include "operators.hpp"

#include <components/detournavigator/navmeshtilescache.hpp>
#include <components/detournavigator/exceptions.hpp>
#include <components/detournavigator/recastmesh.hpp>
#include <components/detournavigator/preparednavmeshdata.hpp>
#include <components/detournavigator/ref.hpp>
#include <components/detournavigator/preparednavmeshdatatuple.hpp>
#include <components/detournavigator/recast.hpp>

#include <osg/Vec3f>

#include <RecastAlloc.h>

#include <gtest/gtest.h>

#include <random>
#include <stdexcept>

namespace
{
    using namespace testing;
    using namespace DetourNavigator;

    void* permRecastAlloc(int size)
    {
        void* result = rcAlloc(static_cast<std::size_t>(size), RC_ALLOC_PERM);
        if (result == nullptr)
            throw std::bad_alloc();
        return result;
    }

    template <class T>
    void generate(T*& values, int size)
    {
        values = static_cast<T*>(permRecastAlloc(size * sizeof(T)));
        std::generate_n(values, static_cast<std::size_t>(size), [] { return static_cast<T>(std::rand()); });
    }

    void generate(rcPolyMesh& value, int size)
    {
        value.nverts = size;
        value.maxpolys = size;
        value.nvp = size;
        value.npolys = size;
        rcVcopy(value.bmin, osg::Vec3f(-1, -2, -3).ptr());
        rcVcopy(value.bmax, osg::Vec3f(3, 2, 1).ptr());
        value.cs = 1.0f / (std::rand() % 999 + 1);
        value.ch = 1.0f / (std::rand() % 999 + 1);
        value.borderSize = std::rand();
        value.maxEdgeError = 1.0f / (std::rand() % 999 + 1);
        generate(value.verts, getVertsLength(value));
        generate(value.polys, getPolysLength(value));
        generate(value.regs, getRegsLength(value));
        generate(value.flags, getFlagsLength(value));
        generate(value.areas, getAreasLength(value));
    }

    void generate(rcPolyMeshDetail& value, int size)
    {
        value.nmeshes = size;
        value.nverts = size;
        value.ntris = size;
        generate(value.meshes, getMeshesLength(value));
        generate(value.verts, getVertsLength(value));
        generate(value.tris, getTrisLength(value));
    }

    void generate(PreparedNavMeshData& value, int size)
    {
        value.mUserId = std::rand();
        value.mCellHeight = 1.0f / (std::rand() % 999 + 1);
        value.mCellSize = 1.0f / (std::rand() % 999 + 1);
        generate(value.mPolyMesh, size);
        generate(value.mPolyMeshDetail, size);
    }

    std::unique_ptr<PreparedNavMeshData> makePeparedNavMeshData(int size)
    {
        auto result = std::make_unique<PreparedNavMeshData>();
        generate(*result, size);
        return result;
    }

    template <class T>
    void clone(const T* src, T*& dst, std::size_t size)
    {
        dst = static_cast<T*>(permRecastAlloc(static_cast<int>(size) * sizeof(T)));
        std::memcpy(dst, src, size * sizeof(T));
    }

    void clone(const rcPolyMesh& src, rcPolyMesh& dst)
    {
        dst.nverts = src.nverts;
        dst.npolys = src.npolys;
        dst.maxpolys = src.maxpolys;
        dst.nvp = src.nvp;
        rcVcopy(dst.bmin, src.bmin);
        rcVcopy(dst.bmax, src.bmax);
        dst.cs = src.cs;
        dst.ch = src.ch;
        dst.borderSize = src.borderSize;
        dst.maxEdgeError = src.maxEdgeError;
        clone(src.verts, dst.verts, getVertsLength(dst));
        clone(src.polys, dst.polys, getPolysLength(dst));
        clone(src.regs, dst.regs, getRegsLength(dst));
        clone(src.flags, dst.flags, getFlagsLength(dst));
        clone(src.areas, dst.areas, getAreasLength(dst));
    }

    void clone(const rcPolyMeshDetail& src, rcPolyMeshDetail& dst)
    {
        dst.nmeshes = src.nmeshes;
        dst.nverts = src.nverts;
        dst.ntris = src.ntris;
        clone(src.meshes, dst.meshes, getMeshesLength(dst));
        clone(src.verts, dst.verts, getVertsLength(dst));
        clone(src.tris, dst.tris, getTrisLength(dst));
    }

    std::unique_ptr<PreparedNavMeshData> clone(const PreparedNavMeshData& value)
    {
        auto result = std::make_unique<PreparedNavMeshData>();
        result->mUserId = value.mUserId;
        result->mCellHeight = value.mCellHeight;
        result->mCellSize = value.mCellSize;
        clone(value.mPolyMesh, result->mPolyMesh);
        clone(value.mPolyMeshDetail, result->mPolyMeshDetail);
        return result;
    }

    Mesh makeMesh()
    {
        std::vector<int> indices {{0, 1, 2}};
        std::vector<float> vertices {{0, 0, 0, 1, 0, 0, 1, 1, 0}};
        std::vector<AreaType> areaTypes {1, AreaType_ground};
        return Mesh(std::move(indices), std::move(vertices), std::move(areaTypes));
    }

    struct DetourNavigatorNavMeshTilesCacheTest : Test
    {
        const osg::Vec3f mAgentHalfExtents {1, 2, 3};
        const TilePosition mTilePosition {0, 0};
        const std::size_t mGeneration = 0;
        const std::size_t mRevision = 0;
        const Mesh mMesh {makeMesh()};
        const std::vector<CellWater> mWater {};
        const std::vector<Heightfield> mHeightfields {};
        const std::vector<FlatHeightfield> mFlatHeightfields {};
        const RecastMesh mRecastMesh {mGeneration, mRevision, mMesh, mWater, mHeightfields, mFlatHeightfields};
        std::unique_ptr<PreparedNavMeshData> mPreparedNavMeshData {makePeparedNavMeshData(3)};

        const std::size_t mRecastMeshSize = sizeof(mRecastMesh) + getSize(mRecastMesh);
        const std::size_t mRecastMeshWithWaterSize = mRecastMeshSize + sizeof(CellWater);
        const std::size_t mPreparedNavMeshDataSize = sizeof(*mPreparedNavMeshData) + getSize(*mPreparedNavMeshData);
    };

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, get_for_empty_cache_should_return_empty_value)
    {
        const std::size_t maxSize = 0;
        NavMeshTilesCache cache(maxSize);

        EXPECT_FALSE(cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, set_for_not_enought_cache_size_should_return_empty_value)
    {
        const std::size_t maxSize = 0;
        NavMeshTilesCache cache(maxSize);

        EXPECT_FALSE(cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, std::move(mPreparedNavMeshData)));
        EXPECT_NE(mPreparedNavMeshData, nullptr);
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, set_should_return_cached_value)
    {
        const std::size_t maxSize = mRecastMeshSize + mPreparedNavMeshDataSize;
        NavMeshTilesCache cache(maxSize);
        const auto copy = clone(*mPreparedNavMeshData);
        ASSERT_EQ(*mPreparedNavMeshData, *copy);

        const auto result = cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, std::move(mPreparedNavMeshData));
        ASSERT_TRUE(result);
        EXPECT_EQ(result.get(), *copy);
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, set_existing_element_should_return_cached_element)
    {
        const std::size_t maxSize = 2 * (mRecastMeshSize + mPreparedNavMeshDataSize);
        NavMeshTilesCache cache(maxSize);
        auto copy = clone(*mPreparedNavMeshData);
        const auto sameCopy = clone(*mPreparedNavMeshData);

        cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, std::move(mPreparedNavMeshData));
        EXPECT_EQ(mPreparedNavMeshData, nullptr);
        const auto result = cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, std::move(copy));
        ASSERT_TRUE(result);
        EXPECT_EQ(result.get(), *sameCopy);
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, get_should_return_cached_value)
    {
        const std::size_t maxSize = mRecastMeshSize + mPreparedNavMeshDataSize;
        NavMeshTilesCache cache(maxSize);
        const auto copy = clone(*mPreparedNavMeshData);

        cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, std::move(mPreparedNavMeshData));
        const auto result = cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh);
        ASSERT_TRUE(result);
        EXPECT_EQ(result.get(), *copy);
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, get_for_cache_miss_by_agent_half_extents_should_return_empty_value)
    {
        const std::size_t maxSize = 1;
        NavMeshTilesCache cache(maxSize);
        const osg::Vec3f unexsistentAgentHalfExtents {1, 1, 1};

        cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, std::move(mPreparedNavMeshData));
        EXPECT_FALSE(cache.get(unexsistentAgentHalfExtents, mTilePosition, mRecastMesh));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, get_for_cache_miss_by_tile_position_should_return_empty_value)
    {
        const std::size_t maxSize = 1;
        NavMeshTilesCache cache(maxSize);
        const TilePosition unexistentTilePosition {1, 1};

        cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, std::move(mPreparedNavMeshData));
        EXPECT_FALSE(cache.get(mAgentHalfExtents, unexistentTilePosition, mRecastMesh));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, get_for_cache_miss_by_recast_mesh_should_return_empty_value)
    {
        const std::size_t maxSize = 1;
        NavMeshTilesCache cache(maxSize);
        const std::vector<CellWater> water(1, CellWater {osg::Vec2i(), Water {1, 0.0f}});
        const RecastMesh unexistentRecastMesh {mGeneration, mRevision, mMesh, water, mHeightfields, mFlatHeightfields};

        cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, std::move(mPreparedNavMeshData));
        EXPECT_FALSE(cache.get(mAgentHalfExtents, mTilePosition, unexistentRecastMesh));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, set_should_replace_unused_value)
    {
        const std::size_t maxSize = mRecastMeshWithWaterSize + mPreparedNavMeshDataSize;
        NavMeshTilesCache cache(maxSize);

        const std::vector<CellWater> water(1, CellWater {osg::Vec2i(), Water {1, 0.0f}});
        const RecastMesh anotherRecastMesh {mGeneration, mRevision, mMesh, water, mHeightfields, mFlatHeightfields};
        auto anotherPreparedNavMeshData = makePeparedNavMeshData(3);
        const auto copy = clone(*anotherPreparedNavMeshData);

        cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, std::move(mPreparedNavMeshData));
        const auto result = cache.set(mAgentHalfExtents, mTilePosition, anotherRecastMesh,
                                      std::move(anotherPreparedNavMeshData));
        ASSERT_TRUE(result);
        EXPECT_EQ(result.get(), *copy);
        EXPECT_FALSE(cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, set_should_not_replace_used_value)
    {
        const std::size_t maxSize = mRecastMeshWithWaterSize + mPreparedNavMeshDataSize;
        NavMeshTilesCache cache(maxSize);

        const std::vector<CellWater> water(1, CellWater {osg::Vec2i(), Water {1, 0.0f}});
        const RecastMesh anotherRecastMesh {mGeneration, mRevision, mMesh, water, mHeightfields, mFlatHeightfields};
        auto anotherPreparedNavMeshData = makePeparedNavMeshData(3);

        const auto value = cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh,
                                     std::move(mPreparedNavMeshData));
        EXPECT_FALSE(cache.set(mAgentHalfExtents, mTilePosition, anotherRecastMesh,
                               std::move(anotherPreparedNavMeshData)));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, set_should_replace_unused_least_recently_set_value)
    {
        const std::size_t maxSize = 2 * (mRecastMeshWithWaterSize + mPreparedNavMeshDataSize);
        NavMeshTilesCache cache(maxSize);
        const auto copy = clone(*mPreparedNavMeshData);

        const std::vector<CellWater> leastRecentlySetWater(1, CellWater {osg::Vec2i(), Water {1, 0.0f}});
        const RecastMesh leastRecentlySetRecastMesh {mGeneration, mRevision, mMesh, leastRecentlySetWater,
                    mHeightfields, mFlatHeightfields};
        auto leastRecentlySetData = makePeparedNavMeshData(3);

        const std::vector<CellWater> mostRecentlySetWater(1, CellWater {osg::Vec2i(), Water {2, 0.0f}});
        const RecastMesh mostRecentlySetRecastMesh {mGeneration, mRevision, mMesh, mostRecentlySetWater,
                    mHeightfields, mFlatHeightfields};
        auto mostRecentlySetData = makePeparedNavMeshData(3);

        ASSERT_TRUE(cache.set(mAgentHalfExtents, mTilePosition, leastRecentlySetRecastMesh,
                              std::move(leastRecentlySetData)));
        ASSERT_TRUE(cache.set(mAgentHalfExtents, mTilePosition, mostRecentlySetRecastMesh,
                              std::move(mostRecentlySetData)));

        const auto result = cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh,
                                      std::move(mPreparedNavMeshData));
        EXPECT_EQ(result.get(), *copy);

        EXPECT_FALSE(cache.get(mAgentHalfExtents, mTilePosition, leastRecentlySetRecastMesh));
        EXPECT_TRUE(cache.get(mAgentHalfExtents, mTilePosition, mostRecentlySetRecastMesh));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, set_should_replace_unused_least_recently_used_value)
    {
        const std::size_t maxSize = 2 * (mRecastMeshWithWaterSize + mPreparedNavMeshDataSize);
        NavMeshTilesCache cache(maxSize);

        const std::vector<CellWater> leastRecentlyUsedWater(1, CellWater {osg::Vec2i(), Water {1, 0.0f}});
        const RecastMesh leastRecentlyUsedRecastMesh {mGeneration, mRevision, mMesh, leastRecentlyUsedWater,
                    mHeightfields, mFlatHeightfields};
        auto leastRecentlyUsedData = makePeparedNavMeshData(3);
        const auto leastRecentlyUsedCopy = clone(*leastRecentlyUsedData);

        const std::vector<CellWater> mostRecentlyUsedWater(1, CellWater {osg::Vec2i(), Water {2, 0.0f}});
        const RecastMesh mostRecentlyUsedRecastMesh {mGeneration, mRevision, mMesh, mostRecentlyUsedWater,
                    mHeightfields, mFlatHeightfields};
        auto mostRecentlyUsedData = makePeparedNavMeshData(3);
        const auto mostRecentlyUsedCopy = clone(*mostRecentlyUsedData);

        cache.set(mAgentHalfExtents, mTilePosition, leastRecentlyUsedRecastMesh, std::move(leastRecentlyUsedData));
        cache.set(mAgentHalfExtents, mTilePosition, mostRecentlyUsedRecastMesh, std::move(mostRecentlyUsedData));

        {
            const auto value = cache.get(mAgentHalfExtents, mTilePosition, leastRecentlyUsedRecastMesh);
            ASSERT_TRUE(value);
            ASSERT_EQ(value.get(), *leastRecentlyUsedCopy);
        }

        {
            const auto value = cache.get(mAgentHalfExtents, mTilePosition, mostRecentlyUsedRecastMesh);
            ASSERT_TRUE(value);
            ASSERT_EQ(value.get(), *mostRecentlyUsedCopy);
        }

        const auto copy = clone(*mPreparedNavMeshData);
        const auto result = cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh,
                                      std::move(mPreparedNavMeshData));
        EXPECT_EQ(result.get(), *copy);

        EXPECT_FALSE(cache.get(mAgentHalfExtents, mTilePosition, leastRecentlyUsedRecastMesh));
        EXPECT_TRUE(cache.get(mAgentHalfExtents, mTilePosition, mostRecentlyUsedRecastMesh));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, set_should_not_replace_unused_least_recently_used_value_when_item_does_not_not_fit_cache_max_size)
    {
        const std::size_t maxSize = 2 * (mRecastMeshWithWaterSize + mPreparedNavMeshDataSize);
        NavMeshTilesCache cache(maxSize);

        const std::vector<CellWater> water(1, CellWater {osg::Vec2i(), Water {1, 0.0f}});
        const RecastMesh tooLargeRecastMesh {mGeneration, mRevision, mMesh, water,
                    mHeightfields, mFlatHeightfields};
        auto tooLargeData = makePeparedNavMeshData(10);

        cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, std::move(mPreparedNavMeshData));
        EXPECT_FALSE(cache.set(mAgentHalfExtents, mTilePosition, tooLargeRecastMesh, std::move(tooLargeData)));
        EXPECT_TRUE(cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, set_should_not_replace_unused_least_recently_used_value_when_item_does_not_not_fit_size_of_unused_items)
    {
        const std::size_t maxSize = 2 * (mRecastMeshWithWaterSize + mPreparedNavMeshDataSize);
        NavMeshTilesCache cache(maxSize);

        const std::vector<CellWater> anotherWater(1, CellWater {osg::Vec2i(), Water {1, 0.0f}});
        const RecastMesh anotherRecastMesh {mGeneration, mRevision, mMesh, anotherWater,
                    mHeightfields, mFlatHeightfields};
        auto anotherData = makePeparedNavMeshData(3);

        const std::vector<CellWater> tooLargeWater(1, CellWater {osg::Vec2i(), Water {2, 0.0f}});
        const RecastMesh tooLargeRecastMesh {mGeneration, mRevision, mMesh, tooLargeWater,
                    mHeightfields, mFlatHeightfields};
        auto tooLargeData = makePeparedNavMeshData(10);

        const auto value = cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh,
                                     std::move(mPreparedNavMeshData));
        ASSERT_TRUE(value);
        ASSERT_TRUE(cache.set(mAgentHalfExtents, mTilePosition, anotherRecastMesh,
                              std::move(anotherData)));
        EXPECT_FALSE(cache.set(mAgentHalfExtents, mTilePosition, tooLargeRecastMesh,
                               std::move(tooLargeData)));
        EXPECT_TRUE(cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh));
        EXPECT_TRUE(cache.get(mAgentHalfExtents, mTilePosition, anotherRecastMesh));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, release_used_after_set_then_used_by_get_item_should_left_this_item_available)
    {
        const std::size_t maxSize = mRecastMeshWithWaterSize + mPreparedNavMeshDataSize;
        NavMeshTilesCache cache(maxSize);

        const std::vector<CellWater> water(1, CellWater {osg::Vec2i(), Water {1, 0.0f}});
        const RecastMesh anotherRecastMesh {mGeneration, mRevision, mMesh, water, mHeightfields, mFlatHeightfields};
        auto anotherData = makePeparedNavMeshData(3);

        const auto firstCopy = cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, std::move(mPreparedNavMeshData));
        ASSERT_TRUE(firstCopy);
        {
            const auto secondCopy = cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh);
            ASSERT_TRUE(secondCopy);
        }
        EXPECT_FALSE(cache.set(mAgentHalfExtents, mTilePosition, anotherRecastMesh, std::move(anotherData)));
        EXPECT_TRUE(cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, release_twice_used_item_should_left_this_item_available)
    {
        const std::size_t maxSize = mRecastMeshWithWaterSize + mPreparedNavMeshDataSize;
        NavMeshTilesCache cache(maxSize);

        const std::vector<CellWater> water(1, CellWater {osg::Vec2i(), Water {1, 0.0f}});
        const RecastMesh anotherRecastMesh {mGeneration, mRevision, mMesh, water, mHeightfields, mFlatHeightfields};
        auto anotherData = makePeparedNavMeshData(3);

        cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, std::move(mPreparedNavMeshData));
        const auto firstCopy = cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh);
        ASSERT_TRUE(firstCopy);
        {
            const auto secondCopy = cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh);
            ASSERT_TRUE(secondCopy);
        }
        EXPECT_FALSE(cache.set(mAgentHalfExtents, mTilePosition, anotherRecastMesh, std::move(anotherData)));
        EXPECT_TRUE(cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh));
    }
}
