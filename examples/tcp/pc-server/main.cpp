/** @file main.cpp
 *
 */
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "PeriodicTask.h"
#include "Transport.h"
#include "TcpServer.h"

//----------------------------------------------------------------------

const unsigned long lDt = 50;          ///< Sampling step [ms]
//----------------------------------------------------------------------

/**
 * @brief Method that calculates the trajectory value and writes the return value in _trajData->dOutput
 * @param _benchData pointer to test rig data struct
 * @param _trajData pointer to trajectory struct
 */
void fTrajectory(struct Transport::benchData *_benchData, struct Transport::trajData *_trajData) {
    _trajData->dRampOutput = _trajData->rampTraj.compute(_benchData->lTime);
    _trajData->dSeriesOutput = _trajData->seriesTraj.compute(_benchData->lTime);
}
//----------------------------------------------------------------------

/**
 * @brief Timer loop method, that implements a control loop
 * @param transport pointer to Transport class instance
 */
void fContLoop(Transport *transport) {
    transport->handleFrames();

    if (transport->runExp()) {
        transport->_benchData.lTime += lDt;

        fTrajectory(&transport->_benchData, &transport->_trajData);

        transport->sendData();
    }
}
//----------------------------------------------------------------------


int main(int argc, char const *argv[]) {
    Queue<Frame> inputQueue;
    Queue<Frame> outputQueue;

    Transport transport(std::ref(inputQueue), std::ref(outputQueue));

    try {
        boost::asio::io_service ioService;

        PeriodicScheduler scheduler(std::ref(ioService));
        scheduler.addTask("fContLoop", boost::bind(fContLoop, &transport), lDt);

        TcpServer server(ioService, std::ref(inputQueue), std::ref(outputQueue), TRANSPORT_TCP_PORT);

        boost::thread_group threads;
        for (int i = 0; i < 2; ++i) {
            threads.create_thread(boost::bind(&boost::asio::io_service::run, &ioService));
        }
        threads.join_all();
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
//----------------------------------------------------------------------
