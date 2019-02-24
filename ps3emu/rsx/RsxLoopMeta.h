#include "../utils.h"

#include <stdint.h>
#include <vector>
#include <type_traits>
#include <functional>

#define BOOST_FUSION_INVOKE_MAX_ARITY 20
#define FUSION_MAX_VECTOR_SIZE 20

#include <boost/function_types/parameter_types.hpp>
#include <boost/fusion/functional/invocation/invoke.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <boost/fusion/include/convert.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/view/joint_view.hpp>
#include <boost/fusion/include/joint_view.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/unpack_args.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/pop_front.hpp>

using namespace boost;
using namespace boost::function_types;

template <typename T> struct DbColumnType {
    static T convert(uint32_t value) {
        return (T)value;
    }
};

template<> struct DbColumnType<float> {
    static float convert(uint32_t value) {
        return bit_cast<float>(value);
    }
};

template<> struct DbColumnType<MemoryLocation> {
    static MemoryLocation convert(uint32_t value) {
        return (MemoryLocation)value;
    }
};

template<typename Vec>
struct SeqBuilder {
    Vec* vec;
    std::vector<uint32_t>* src;
    template<typename U> void operator()(U x) {
        constexpr auto i = U::value;
        typedef typename std::remove_reference<
            typename fusion::result_of::at_c<Vec, i>::type
        >::type dest_t;
        fusion::at_c<i>(*vec) = DbColumnType< dest_t>::convert( (*src)[x - 1] );
    }
};

class Rsx;

template<typename F>
void replay(F f, Rsx* rsx, std::vector<uint32_t>& args) {
    typedef typename fusion::result_of::as_vector<
        fusion::joint_view<
            mpl::vector<Rsx*>,
            typename mpl::pop_front< parameter_types<F> >::type
        >
    >::type vec_t;
    
    constexpr auto size = mpl::size< vec_t >::value;
    typedef mpl::range_c<int, 1, size> num_seq;
    
    vec_t vec;
    fusion::at_c<0>(vec) = rsx;
    SeqBuilder<vec_t> builder;
    builder.vec = &vec;
    builder.src = &args;
    mpl::for_each<num_seq>(builder);
    fusion::invoke(f, vec);
}
