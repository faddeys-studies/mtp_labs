#ifndef MTP_LAB1_COMMON_H
#define MTP_LAB1_COMMON_H

#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
    #include <mutex>
    #include <iostream>
    std::mutex __cerr_mutex;
    #define debug(args) {\
        std::unique_lock<std::mutex> _lk{__cerr_mutex}; \
        std::cerr << std::this_thread::get_id() << ": " << args << std::endl; \
    }
#else
    #define debug(args)
#endif


#endif //MTP_LAB1_COMMON_H
