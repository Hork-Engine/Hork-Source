#include "RectangleBinPack.h"
#include <Core/BaseMath.h>

#include <limits>
#include <memory.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The following functions compute (penalty) score values if a rect of the given size was placed into the
// given free rectangle. In these score values, smaller is better.

static int __ScoreBestAreaFit( int _Width, int _Height, const SRectangleBinBack_RectNode & _FreeRect ) {
    return _FreeRect.width * _FreeRect.height - _Width * _Height;
}

static int __ScoreBestShortSideFit( int _Width, int _Height, const SRectangleBinBack_RectNode &_FreeRect ) {
    int leftoverHoriz = Math::Abs( _FreeRect.width - _Width );
    int leftoverVert = Math::Abs( _FreeRect.height - _Height );
    int leftover = Math::Min( leftoverHoriz, leftoverVert );
    return leftover;
}

static int __ScoreBestintSideFit( int _Width, int _Height, const SRectangleBinBack_RectNode &_FreeRect ) {
    int leftoverHoriz = Math::Abs( _FreeRect.width - _Width );
    int leftoverVert = Math::Abs( _FreeRect.height - _Height );
    int leftover = Math::Max( leftoverHoriz, leftoverVert );
    return leftover;
}

static int __ScoreWorstAreaFit( int _Width, int _Height, const SRectangleBinBack_RectNode &_FreeRect ) {
    return -__ScoreBestAreaFit( _Width, _Height, _FreeRect );
}

static int __ScoreWorstShortSideFit( int _Width, int _Height, const SRectangleBinBack_RectNode &_FreeRect ) {
    return -__ScoreBestShortSideFit( _Width, _Height, _FreeRect );
}

static int __ScoreWorstintSideFit( int _Width, int _Height, const SRectangleBinBack_RectNode &_FreeRect ) {
    return -__ScoreBestintSideFit( _Width, _Height, _FreeRect );
}

/// Returns the heuristic score value for placing a rectangle of size width*height into freeRect. Does not try to rotate.
static int __ScoreByHeuristic( int _Width, int _Height, const SRectangleBinBack_RectNode & _FreeRect, ARectangleBinPack::FreeRectChoiceHeuristic _RectChoice ) {
    switch ( _RectChoice ) {
    case ARectangleBinPack::RectBestAreaFit:
        return __ScoreBestAreaFit( _Width, _Height, _FreeRect );
    case ARectangleBinPack::RectBestShortSideFit:
        return __ScoreBestShortSideFit( _Width, _Height, _FreeRect );
    case ARectangleBinPack::RectBestintSideFit:
        return __ScoreBestintSideFit( _Width, _Height, _FreeRect );
    case ARectangleBinPack::RectWorstAreaFit:
        return __ScoreWorstAreaFit( _Width, _Height, _FreeRect );
    case ARectangleBinPack::RectWorstShortSideFit:
        return __ScoreWorstShortSideFit( _Width, _Height, _FreeRect );
    case ARectangleBinPack::RectWorstintSideFit:
        return __ScoreWorstintSideFit( _Width, _Height, _FreeRect );
    default:
        AN_ASSERT( 0 );
        return std::numeric_limits<int>::max();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ARectangleBinPack::ARectangleBinPack( int _Width, int _Height ) {
    m_BinWidth = _Width;
    m_BinHeight = _Height;

//#ifdef _DEBUG
    m_DisjointRects.Clear();
//#endif

    // Clear any memory of previously packed rectangles.
    m_UsedRectangles.clear();

    // We start with a single big free rectangle that spans the whole bin.
    SRectangleBinBack_RectNode n;
    n.x = 0;
    n.y = 0;
    n.width = _Width;
    n.height = _Height;

    m_FreeRectangles.clear();
    m_FreeRectangles.push_back( n );
}

void ARectangleBinPack::Insert( std::vector<SRectSize> & _Rects, /*std::vector<Rect> &dst, */bool _Merge,
                      FreeRectChoiceHeuristic _RectChoice, SplitHeuristic _SplitMethod, bool _AllowFlip ) {
    //dst.clear();

    // Remember variables about the best packing choice we have made so far during the iteration process.
    int bestFreeRect = 0;
    int bestRect = 0;
    bool bestFlipped = false;

    // Pack rectangles one at a time until we have cleared the rects array of all rectangles.
    // rects will get destroyed in the process.
    while ( _Rects.size() > 0 ) {
        // Stores the penalty score of the best rectangle placement - bigger=worse, smaller=better.
        int bestScore = std::numeric_limits<int>::max();

        for ( size_t i = 0 ; i < m_FreeRectangles.size() ; ++i ) {
            for ( size_t j = 0; j < _Rects.size() ; ++j ) {
                // If this rectangle is a perfect match, we pick it instantly.
                if ( _Rects[j].width == m_FreeRectangles[i].width && _Rects[j].height == m_FreeRectangles[i].height ) {
                    bestFreeRect = i;
                    bestRect = j;
                    bestFlipped = false;
                    bestScore = std::numeric_limits<int>::min();
                    i = m_FreeRectangles.size(); // Force a jump out of the outer loop as well - we got an instant fit.
                    break;
                }
                // If flipping this rectangle is a perfect match, pick that then.
                else if ( _AllowFlip && _Rects[j].height == m_FreeRectangles[i].width && _Rects[j].width == m_FreeRectangles[i].height) {
                    bestFreeRect = i;
                    bestRect = j;
                    bestFlipped = true;
                    bestScore = std::numeric_limits<int>::min();
                    i = m_FreeRectangles.size(); // Force a jump out of the outer loop as well - we got an instant fit.
                    break;
                }
                // Try if we can fit the rectangle upright.
                else if ( _Rects[j].width <= m_FreeRectangles[i].width && _Rects[j].height <= m_FreeRectangles[i].height ) {
                    int score = __ScoreByHeuristic( _Rects[j].width, _Rects[j].height, m_FreeRectangles[i], _RectChoice );
                    if ( score < bestScore ) {
                        bestFreeRect = i;
                        bestRect = j;
                        bestFlipped = false;
                        bestScore = score;
                    }
                }
                // If not, then perhaps flipping sideways will make it fit?
                else if ( _AllowFlip && _Rects[j].height <= m_FreeRectangles[i].width && _Rects[j].width <= m_FreeRectangles[i].height ) {
                    int score = __ScoreByHeuristic( _Rects[j].height, _Rects[j].width, m_FreeRectangles[i], _RectChoice );
                    if ( score < bestScore ) {
                        bestFreeRect = i;
                        bestRect = j;
                        bestFlipped = true;
                        bestScore = score;
                    }
                }
            }
        }

        // If we didn't manage to find any rectangle to pack, abort.
        if ( bestScore == std::numeric_limits<int>::max() ) {
            return;
        }

        // Otherwise, we're good to go and do the actual packing.
        SRectangleBinBack_RectNode newNode;
        newNode.x = m_FreeRectangles[bestFreeRect].x;
        newNode.y = m_FreeRectangles[bestFreeRect].y;
        newNode.width = _Rects[bestRect].width;
        newNode.height = _Rects[bestRect].height;
        newNode.userdata = _Rects[bestRect].userdata;

        if ( bestFlipped ) {
            std::swap( newNode.width, newNode.height );
        }

        // Remove the free space we lost in the bin.
        SplitFreeRectByHeuristic( m_FreeRectangles[bestFreeRect], newNode, _SplitMethod );
        m_FreeRectangles.erase( m_FreeRectangles.begin() + bestFreeRect );

        // Remove the rectangle we just packed from the input list.
        _Rects.erase( _Rects.begin() + bestRect );

        // Perform a Rectangle Merge step if desired.
        if ( _Merge ) {
            MergeFreeList();
        }

        // Remember the new used rectangle.
        m_UsedRectangles.push_back( newNode );

        // Check that we're really producing correct packings here.
        AN_ASSERT( m_DisjointRects.Add( newNode ) == true );
    }
}

/*
/// @return True if r fits inside freeRect (possibly rotated).
bool __Fits( const FRectangleBinBack_RectNode & r, const FRectangleBinBack_RectNode & freeRect ) {
    return ( r.width <= freeRect.width && r.height <= freeRect.height ) ||
        ( r.height <= freeRect.width && r.width <= freeRect.height );
}

/// @return True if r fits perfectly inside freeRect, i.e. the leftover area is 0.
static bool __FitsPerfectly( const FRectangleBinBack_RectNode & r, const FRectangleBinBack_RectNode & freeRect ) {
    return ( r.width == freeRect.width && r.height == freeRect.height ) ||
        ( r.height == freeRect.width && r.width == freeRect.height );
}

// A helper function for GUILLOTINE-MAXFITTING. Counts how many rectangles fit into the given rectangle
// after it has been split.
void CountNumFitting(const Rect &freeRect, int width, int height, const std::vector<RectSize> &rects,
    int usedRectIndex, bool splitHorizontal, int &score1, int &score2)
{
    const int w = freeRect.width - width;
    const int h = freeRect.height - height;

    Rect bottom;
    bottom.x = freeRect.x;
    bottom.y = freeRect.y + height;
    bottom.height = h;

    Rect right;
    right.x = freeRect.x + width;
    right.y = freeRect.y;
    right.width = w;

    if (splitHorizontal)
    {
        bottom.width = freeRect.width;
        right.height = height;
    }
    else // Split vertically
    {
        bottom.width = width;
        right.height = freeRect.height;
    }

    int fitBottom = 0;
    int fitRight = 0;
    for(size_t i = 0; i < rects.size(); ++i)
        if (i != usedRectIndex)
        {
            if (__FitsPerfectly(rects[i], bottom))
                fitBottom |= 0x10000000;
            if (__FitsPerfectly(rects[i], right))
                fitRight |= 0x10000000;

            if (Fits(rects[i], bottom))
                ++fitBottom;
            if (Fits(rects[i], right))
                ++fitRight;
        }

    score1 = min(fitBottom, fitRight);
    score2 = max(fitBottom, fitRight);
}
*/
/*
// Implements GUILLOTINE-MAXFITTING, an experimental heuristic that's really cool but didn't quite work in practice.
void FRectangleBinPack::InsertMaxFitting(std::vector<RectSize> &rects, std::vector<Rect> &dst, bool merge,
    FreeRectChoiceHeuristic rectChoice, SplitHeuristic splitMethod)
{
    dst.clear();
    int bestRect = 0;
    bool bestFlipped = false;
    bool bestSplitHorizontal = false;

    // Pick rectangles one at a time and pack the one that leaves the most choices still open.
    while(rects.size() > 0 && freeRectangles.size() > 0)
    {
        int bestScore1 = -1;
        int bestScore2 = -1;

        ///\todo Different sort predicates.
        clb::sort::QuickSort(&freeRectangles[0], freeRectangles.size(), CompareRectShortSide);

        Rect &freeRect = freeRectangles[0];

        for(size_t j = 0; j < rects.size(); ++j)
        {
            int score1;
            int score2;

            if (rects[j].width == freeRect.width && rects[j].height == freeRect.height)
            {
                bestRect = j;
                bestFlipped = false;
                bestScore1 = bestScore2 = std::numeric_limits<int>::max();
                break;
            }
            else if (rects[j].width <= freeRect.width && rects[j].height <= freeRect.height)
            {
                CountNumFitting(freeRect, rects[j].width, rects[j].height, rects, j, false, score1, score2);

                if (score1 > bestScore1 || (score1 == bestScore1 && score2 > bestScore2))
                {
                    bestRect = j;
                    bestScore1 = score1;
                    bestScore2 = score2;
                    bestFlipped = false;
                    bestSplitHorizontal = false;
                }

                CountNumFitting(freeRect, rects[j].width, rects[j].height, rects, j, true, score1, score2);

                if (score1 > bestScore1 || (score1 == bestScore1 && score2 > bestScore2))
                {
                    bestRect = j;
                    bestScore1 = score1;
                    bestScore2 = score2;
                    bestFlipped = false;
                    bestSplitHorizontal = true;
                }
            }

            if (rects[j].height == freeRect.width && rects[j].width == freeRect.height)
            {
                bestRect = j;
                bestFlipped = true;
                bestScore1 = bestScore2 = std::numeric_limits<int>::max();
                break;
            }
            else if (rects[j].height <= freeRect.width && rects[j].width <= freeRect.height)
            {
                CountNumFitting(freeRect, rects[j].height, rects[j].width, rects, j, false, score1, score2);

                if (score1 > bestScore1 || (score1 == bestScore1 && score2 > bestScore2))
                {
                    bestRect = j;
                    bestScore1 = score1;
                    bestScore2 = score2;
                    bestFlipped = true;
                    bestSplitHorizontal = false;
                }

                CountNumFitting(freeRect, rects[j].height, rects[j].width, rects, j, true, score1, score2);

                if (score1 > bestScore1 || (score1 == bestScore1 && score2 > bestScore2))
                {
                    bestRect = j;
                    bestScore1 = score1;
                    bestScore2 = score2;
                    bestFlipped = true;
                    bestSplitHorizontal = true;
                }
            }
        }

        if (bestScore1 >= 0)
        {
            Rect newNode;
            newNode.x = freeRect.x;
            newNode.y = freeRect.y;
            newNode.width = rects[bestRect].width;
            newNode.height = rects[bestRect].height;
            if (bestFlipped)
                std::swap(newNode.width, newNode.height);

            AN_ASSERT(disjointRects.Disjoint(newNode));
            SplitFreeRectAintAxis(freeRect, newNode, bestSplitHorizontal);

            rects.erase(rects.begin() + bestRect);

            if (merge)
                MergeFreeList();

            usedRectangles.push_back(newNode);
#ifdef _DEBUG
            disjointRects.Add(newNode);
#endif
        }

        freeRectangles.erase(freeRectangles.begin());
    }
}
*/

SRectangleBinBack_RectNode ARectangleBinPack::Insert( int _Width, int _Height, bool _Merge, FreeRectChoiceHeuristic _RectChoice,
                            SplitHeuristic _SplitMethod ) {
    // Find where to put the new rectangle.
    int freeNodeIndex = 0;
    SRectangleBinBack_RectNode newRect = FindPositionForNewNode( _Width, _Height, _RectChoice, &freeNodeIndex );

    newRect.userdata = NULL;

    // Abort if we didn't have enough space in the bin.
    if ( newRect.height == 0 ) {
        return newRect;
    }

    // Remove the space that was just consumed by the new rectangle.
    SplitFreeRectByHeuristic( m_FreeRectangles[freeNodeIndex], newRect, _SplitMethod );
    m_FreeRectangles.erase( m_FreeRectangles.begin() + freeNodeIndex );

    // Perform a Rectangle Merge step if desired.
    if ( _Merge ) {
        MergeFreeList();
    }

    // Remember the new used rectangle.
    m_UsedRectangles.push_back( newRect );

    // Check that we're really producing correct packings here.
    AN_ASSERT( m_DisjointRects.Add( newRect ) == true );

    return newRect;
}

/// Computes the ratio of used surface area to the total bin area.
float ARectangleBinPack::Occupancy() const {
    ///\todo The occupancy rate could be cached/tracked incrementally instead
    ///      of looping through the list of packed rectangles here.
    unsigned int usedSurfaceArea = 0;
    for ( size_t i = 0 ; i < m_UsedRectangles.size() ; ++i ) {
        usedSurfaceArea += m_UsedRectangles[i].width * m_UsedRectangles[i].height;
    }

    return (float)usedSurfaceArea / ( m_BinWidth * m_BinHeight );
}

SRectangleBinBack_RectNode ARectangleBinPack::FindPositionForNewNode( int _Width, int _Height, FreeRectChoiceHeuristic _RectChoice, int * _NodeIndex ) {
    SRectangleBinBack_RectNode bestNode;

    Platform::ZeroMem(&bestNode, sizeof(SRectangleBinBack_RectNode));

    int bestScore = std::numeric_limits<int>::max();

    /// Try each free rectangle to find the best one for placement.
    for ( size_t i = 0 ; i < m_FreeRectangles.size() ; ++i ) {
        // If this is a perfect fit upright, choose it immediately.
        if ( _Width == m_FreeRectangles[i].width && _Height == m_FreeRectangles[i].height ) {
            bestNode.x = m_FreeRectangles[i].x;
            bestNode.y = m_FreeRectangles[i].y;
            bestNode.width = _Width;
            bestNode.height = _Height;
            bestScore = std::numeric_limits<int>::min();
            *_NodeIndex = i;
            AN_ASSERT( m_DisjointRects.Disjoint( bestNode ) );
            break;
        }
        // If this is a perfect fit sideways, choose it.
        else if ( _Height == m_FreeRectangles[i].width && _Width == m_FreeRectangles[i].height ) {
            bestNode.x = m_FreeRectangles[i].x;
            bestNode.y = m_FreeRectangles[i].y;
            bestNode.width = _Height;
            bestNode.height = _Width;
            bestScore = std::numeric_limits<int>::min();
            *_NodeIndex = i;
            AN_ASSERT( m_DisjointRects.Disjoint( bestNode ) );
            break;
        }
        // Does the rectangle fit upright?
        else if ( _Width <= m_FreeRectangles[i].width && _Height <= m_FreeRectangles[i].height ) {
            int score = __ScoreByHeuristic( _Width, _Height, m_FreeRectangles[i], _RectChoice );

            if ( score < bestScore ) {
                bestNode.x = m_FreeRectangles[i].x;
                bestNode.y = m_FreeRectangles[i].y;
                bestNode.width = _Width;
                bestNode.height = _Height;
                bestScore = score;
                *_NodeIndex = i;
                AN_ASSERT( m_DisjointRects.Disjoint(bestNode) );
            }
        }
        // Does the rectangle fit sideways?
        else if ( _Height <= m_FreeRectangles[i].width && _Width <= m_FreeRectangles[i].height ) {
            int score = __ScoreByHeuristic( _Height, _Width, m_FreeRectangles[i], _RectChoice );

            if ( score < bestScore ) {
                bestNode.x = m_FreeRectangles[i].x;
                bestNode.y = m_FreeRectangles[i].y;
                bestNode.width = _Height;
                bestNode.height = _Width;
                bestScore = score;
                *_NodeIndex = i;
                AN_ASSERT( m_DisjointRects.Disjoint( bestNode ) );
            }
        }
    }
    return bestNode;
}

void ARectangleBinPack::SplitFreeRectByHeuristic( const SRectangleBinBack_RectNode & _FreeRect, const SRectangleBinBack_RectNode & _PlacedRect, SplitHeuristic _Method ) {
    // Compute the lengths of the leftover area.
    const int w = _FreeRect.width - _PlacedRect.width;
    const int h = _FreeRect.height - _PlacedRect.height;

    // Placing placedRect into _FreeRect results in an L-shaped free area, which must be split into
    // two disjoint rectangles. This can be achieved with by splitting the L-shape using a single line.
    // We have two choices: horizontal or vertical.

    // Use the given heuristic to decide which choice to make.

    bool splitHorizontal;
    switch ( _Method ) {
    case SplitShorterLeftoverAxis:
        // Split aint the shorter leftover axis.
        splitHorizontal = (w <= h);
        break;
    case SplitinterLeftoverAxis:
        // Split aint the inter leftover axis.
        splitHorizontal = (w > h);
        break;
    case SplitMinimizeArea:
        // Maximize the larger area == minimize the smaller area.
        // Tries to make the single bigger rectangle.
        splitHorizontal = (_PlacedRect.width * h > w * _PlacedRect.height);
        break;
    case SplitMaximizeArea:
        // Maximize the smaller area == minimize the larger area.
        // Tries to make the rectangles more even-sized.
        splitHorizontal = (_PlacedRect.width * h <= w * _PlacedRect.height);
        break;
    case SplitShorterAxis:
        // Split aint the shorter total axis.
        splitHorizontal = (_FreeRect.width <= _FreeRect.height);
        break;
    case SplitinterAxis:
        // Split aint the inter total axis.
        splitHorizontal = (_FreeRect.width > _FreeRect.height);
        break;
    default:
        splitHorizontal = true;
        AN_ASSERT(0);
    }

    // Perform the actual split.
    SplitFreeRectAintAxis( _FreeRect, _PlacedRect, splitHorizontal );
}

/// This function will add the two generated rectangles into the freeRectangles array. The caller is expected to
/// remove the original rectangle from the freeRectangles array after that.
void ARectangleBinPack::SplitFreeRectAintAxis( const SRectangleBinBack_RectNode & _FreeRect, const SRectangleBinBack_RectNode & _PlacedRect, bool _SplitHorizontal ) {
    // Form the two new rectangles.
    SRectangleBinBack_RectNode bottom;
    bottom.x = _FreeRect.x;
    bottom.y = _FreeRect.y + _PlacedRect.height;
    bottom.height = _FreeRect.height - _PlacedRect.height;

    SRectangleBinBack_RectNode right;
    right.x = _FreeRect.x + _PlacedRect.width;
    right.y = _FreeRect.y;
    right.width = _FreeRect.width - _PlacedRect.width;

    if ( _SplitHorizontal ) {
        bottom.width = _FreeRect.width;
        right.height = _PlacedRect.height;
    } else {
        // Split vertically
        bottom.width = _PlacedRect.width;
        right.height = _FreeRect.height;
    }

    // Add the new rectangles into the free rectangle pool if they weren't degenerate.
    if ( bottom.width > 0 && bottom.height > 0 ) {
        m_FreeRectangles.push_back( bottom );
    }

    if ( right.width > 0 && right.height > 0 ) {
        m_FreeRectangles.push_back( right );
    }

    AN_ASSERT( m_DisjointRects.Disjoint( bottom ) );
    AN_ASSERT( m_DisjointRects.Disjoint( right ) );
}

void ARectangleBinPack::MergeFreeList() {
//#ifdef _DEBUG
    ADisjointRectCollection test;
    for ( size_t i = 0 ; i < m_FreeRectangles.size() ; ++i ) {
        AN_ASSERT( test.Add( m_FreeRectangles[i] ) == true );
    }
//#endif

    // Do a Theta(n^2) loop to see if any pair of free rectangles could me merged into one.
    // Note that we miss any opportunities to merge three rectangles into one. (should call this function again to detect that)
    for ( size_t i = 0 ; i < m_FreeRectangles.size() ; ++i ) {
        for ( size_t j = i+1; j < m_FreeRectangles.size(); ++j ) {
            if ( m_FreeRectangles[i].width == m_FreeRectangles[j].width && m_FreeRectangles[i].x == m_FreeRectangles[j].x ) {
                if ( m_FreeRectangles[i].y == m_FreeRectangles[j].y + m_FreeRectangles[j].height ) {
                    m_FreeRectangles[i].y -= m_FreeRectangles[j].height;
                    m_FreeRectangles[i].height += m_FreeRectangles[j].height;
                    m_FreeRectangles.erase( m_FreeRectangles.begin() + j );
                    --j;
                } else if ( m_FreeRectangles[i].y + m_FreeRectangles[i].height == m_FreeRectangles[j].y ) {
                    m_FreeRectangles[i].height += m_FreeRectangles[j].height;
                    m_FreeRectangles.erase( m_FreeRectangles.begin() + j );
                    --j;
                }
            } else if ( m_FreeRectangles[i].height == m_FreeRectangles[j].height && m_FreeRectangles[i].y == m_FreeRectangles[j].y ) {
                if ( m_FreeRectangles[i].x == m_FreeRectangles[j].x + m_FreeRectangles[j].width ) {
                    m_FreeRectangles[i].x -= m_FreeRectangles[j].width;
                    m_FreeRectangles[i].width += m_FreeRectangles[j].width;
                    m_FreeRectangles.erase( m_FreeRectangles.begin() + j );
                    --j;
                } else if ( m_FreeRectangles[i].x + m_FreeRectangles[i].width == m_FreeRectangles[j].x ) {
                    m_FreeRectangles[i].width += m_FreeRectangles[j].width;
                    m_FreeRectangles.erase( m_FreeRectangles.begin() + j );
                    --j;
                }
            }
        }
    }

//#ifdef _DEBUG
    test.Clear();
    for ( size_t i = 0 ; i < m_FreeRectangles.size() ; ++i ) {
        AN_ASSERT( test.Add( m_FreeRectangles[i] ) == true );
    }
//#endif
}
