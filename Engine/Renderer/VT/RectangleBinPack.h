#pragma once

#include <Engine/Core/BaseTypes.h>

#include <vector>

HK_NAMESPACE_BEGIN

struct RectangleBinBack_RectNode
{
    int x;
    int y;
    int width;
    int height;
    bool transposed;

    void* userdata;
};

class DisjointRectCollection
{
public:
    std::vector<RectangleBinBack_RectNode> rects;

    bool Add(const RectangleBinBack_RectNode& r)
    {
        // Игнорируем вырожденные ректанглы
        if (r.width == 0 || r.height == 0)
        {
            return true;
        }

        if (!Disjoint(r))
        {
            return false;
        }

        rects.push_back(r);
        return true;
    }

    void Clear()
    {
        rects.clear();
    }

    bool Disjoint(const RectangleBinBack_RectNode& r) const
    {
        // Игнорируем вырожденные ректанглы
        if (r.width == 0 || r.height == 0)
        {
            return true;
        }

        for (int i = 0; i < static_cast<int>(rects.size()); ++i)
        {
            if (!sDisjoint(rects[i], r))
            {
                return false;
            }
        }
        return true;
    }

    static bool sDisjoint(const RectangleBinBack_RectNode& a, const RectangleBinBack_RectNode& b)
    {
        if (a.x + a.width <= b.x || b.x + b.width <= a.x || a.y + a.height <= b.y || b.y + b.height <= a.y)
        {
            return true;
        }
        return false;
    }
};

class RectangleBinPack
{
public:
    RectangleBinPack(int _Width, int _Height);

    /// Specifies the different choice heuristics that can be used when deciding which of the free subrectangles
    /// to place the to-be-packed rectangle into.
    enum FreeRectChoiceHeuristic
    {
        RectBestAreaFit,
        RectBestShortSideFit,
        RectBestintSideFit,
        RectWorstAreaFit,
        RectWorstShortSideFit,
        RectWorstintSideFit
    };

    /// Specifies the different choice heuristics that can be used when the packer needs to decide whether to
    /// subdivide the remaining free space in horizontal or vertical direction.
    enum SplitHeuristic
    {
        SplitShorterLeftoverAxis,
        SplitinterLeftoverAxis,
        SplitMinimizeArea, /// Try to make a single big rectangle at the expense of making the other small.
        SplitMaximizeArea, /// Try to make both remaining rectangles as even-sized as possible.
        SplitShorterAxis,
        SplitinterAxis
    };

    struct RectSize
    {
        int width;
        int height;

        void* userdata;
    };

    /// Inserts a single rectangle into the bin. The packer might rotate the rectangle, in which case the returned
    /// struct will have the width and height values swapped.
    /// @param merge If true, performs free Rectangle Merge procedure after packing the new rectangle. This procedure
    ///		tries to defragment the list of disjoint free rectangles to improve packing performance, but also takes up
    ///		some extra time.
    /// @param rectChoice The free rectangle choice heuristic rule to use.
    /// @param splitMethod The free rectangle split heuristic rule to use.
    RectangleBinBack_RectNode Insert(int _Width, int _Height, bool _Merge, FreeRectChoiceHeuristic _RectChoice, SplitHeuristic _SplitMethod);

    /// Inserts a list of rectangles into the bin.
    /// @param rects The list of rectangles to add. This list will be destroyed in the packing process.
    /// @param dst The outputted list of rectangles. Note that the indices will not correspond to the input indices.
    /// @param merge If true, performs Rectangle Merge operations during the packing process.
    /// @param rectChoice The free rectangle choice heuristic rule to use.
    /// @param splitMethod The free rectangle split heuristic rule to use.
    void Insert(std::vector<RectSize>& _Rects /*, std::vector<Rect> &dst*/, bool _Merge, FreeRectChoiceHeuristic _RectChoice, SplitHeuristic _SplitMethod, bool _AllowFlip);

    // Implements GUILLOTINE-MAXFITTING, an experimental heuristic that's really cool but didn't quite work in practice.
    //	void InsertMaxFitting(std::vector<RectSize> &rects, std::vector<Rect> &dst, bool merge,
    //		FreeRectChoiceHeuristic rectChoice, SplitHeuristic splitMethod);

    /// Computes the ratio of used/total surface area. 0.00 means no space is yet used, 1.00 means the whole bin is used.
    float Occupancy() const;

    /// Returns the internal list of disjoint rectangles that track the free area of the bin. You may alter this vector
    /// any way desired, as int as the end result still is a list of disjoint rectangles.
    std::vector<RectangleBinBack_RectNode>& GetFreeRectangles() { return m_FreeRectangles; }

    /// Returns the list of packed rectangles. You may alter this vector at will, for example, you can move a Rect from
    /// this list to the Free Rectangles list to free up space on-the-fly, but notice that this causes fragmentation.
    std::vector<RectangleBinBack_RectNode>& GetUsedRectangles() { return m_UsedRectangles; }

    /// Performs a Rectangle Merge operation. This procedure looks for adjacent free rectangles and merges them if they
    /// can be represented with a single rectangle. Takes up Theta(|freeRectangles|^2) time.
    void MergeFreeList();

private:
    int m_BinWidth;
    int m_BinHeight;

    /// Stores a list of all the rectangles that we have packed so far. This is used only to compute the Occupancy ratio,
    /// so if you want to have the packer consume less memory, this can be removed.
    std::vector<RectangleBinBack_RectNode> m_UsedRectangles;

    /// Stores a list of rectangles that represents the free area of the bin. This rectangles in this list are disjoint.
    std::vector<RectangleBinBack_RectNode> m_FreeRectangles;

    //#ifdef _DEBUG
    /// Used to track that the packer produces proper packings.
    DisjointRectCollection m_DisjointRects;
    //#endif

    /// Goes through the list of free rectangles and finds the best one to place a rectangle of given size into.
    /// Running time is Theta(|freeRectangles|).
    /// @param nodeIndex [out] The index of the free rectangle in the freeRectangles array into which the new
    ///		rect was placed.
    /// @return A Rect structure that represents the placement of the new rect into the best free rectangle.
    RectangleBinBack_RectNode FindPositionForNewNode(int _Width, int _Height, FreeRectChoiceHeuristic _RectChoice, int* _NodeIndex);

    /// Splits the given L-shaped free rectangle into two new free rectangles after placedRect has been placed into it.
    /// Determines the split axis by using the given heuristic.
    void SplitFreeRectByHeuristic(const RectangleBinBack_RectNode& _FreeRect, const RectangleBinBack_RectNode& _PlacedRect, SplitHeuristic _Method);

    /// Splits the given L-shaped free rectangle into two new free rectangles aint the given fixed split axis.
    void SplitFreeRectAintAxis(const RectangleBinBack_RectNode& _FreeRect, const RectangleBinBack_RectNode& _PlacedRect, bool _SplitHorizontal);
};

HK_NAMESPACE_END
