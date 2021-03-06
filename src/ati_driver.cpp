/*
 * Copyright (c) 2015, Robot Control and Pattern Recognition Group, Warsaw University of Technology.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Robot Control and Pattern Recognition Group,
 *       Warsaw University of Technology nor the names of its contributors may
 *       be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <rtt/plugin/ServicePlugin.hpp>

#include <ec_hardware/ECDriver.h>
#include <ec_hardware/ECDriverFactory.h>
#include <ec_hardware/ECPDOEntry.h>

#include "geometry_msgs/Wrench.h"

class ATIDriver : public ECDriver {
 public:
  enum ControlMode {PROFILE_POSITION = 1, PROFILE_VELOCITY = 2, PROFILE_CURRENT = 3, HOMING = 6, CYCLIC_CURRENT = 10, CYCLIC_VELOCITY = 9, CYCLIC_POSITION = 8};
  enum ServoState {INVALID = 0, NOT_READY_TO_SWITCH_ON = 1, SWITCH_ON_DISABLED = 2, READY_TO_SWITCH_ON = 3, SWITCH_ON = 4, OPERATION_ENABLED = 5, QUICK_STOP_ACTIVE = 6, FAULT_REACTION_ACTIVE = 7, FAULT = 8};

  ATIDriver(const std::string &name)
      : ECDriver(name),
        force_x_pdo_(0x6000, 1),
        force_y_pdo_(0x6000, 2),
        force_z_pdo_(0x6000, 3),
        torque_x_pdo_(0x6000, 4),
        torque_y_pdo_(0x6000, 5),
        torque_z_pdo_(0x6000, 6),
        control1_pdo_(0x7010, 1),
        bias_(true),
        calib_(0),
        filter_(0) {

    this->provides()->addPort("wrench", wrench_port_);

    this->provides()->addOperation("bias", &ATIDriver::bias, this, RTT::OwnThread);
    this->provides()->addOperation("setCalibration", &ATIDriver::setCalib, this, RTT::OwnThread);
    this->provides()->addOperation("setFilter", &ATIDriver::setFilter, this, RTT::OwnThread);
  }

  ~ATIDriver() {
  }

  virtual bool configureHook(const YAML::Node &cfg) {
    this->addPDOEntry(&force_x_pdo_);
    this->addPDOEntry(&force_y_pdo_);
    this->addPDOEntry(&force_z_pdo_);
    this->addPDOEntry(&torque_x_pdo_);
    this->addPDOEntry(&torque_y_pdo_);
    this->addPDOEntry(&torque_z_pdo_);
    this->addPDOEntry(&control1_pdo_);
    return true;
  }

  virtual void updateInputs() {
    int32_t fx, fy, fz, tx, ty, tz;
    geometry_msgs::Wrench wr;

    fx = force_x_pdo_.read();
    fy = force_y_pdo_.read();
    fz = force_z_pdo_.read();

    tx = torque_x_pdo_.read();
    ty = torque_y_pdo_.read();
    tz = torque_z_pdo_.read();

    wr.force.x = (double)fx/1000000.0;
    wr.force.y = (double)fy/1000000.0;
    wr.force.z = (double)fz/1000000.0;

    wr.torque.x = (double)tx/1000000.0;
    wr.torque.y = (double)ty/1000000.0;
    wr.torque.z = (double)tz/1000000.0;

    wrench_port_.write(wr);
  }

  virtual void updateOutputs() {
    int32_t cw1 = 0;

    if (bias_) {
      cw1 |= 1;
      bias_ = false;
    }

    cw1 |= filter_<<4;
    cw1 |= calib_<<8;

    control1_pdo_.write(cw1);
  }

 private:

  void bias() {
    bias_ = true;
  }

  bool setFilter(int32_t fl) {
    if (fl < 0 || fl > 8) {
      return false;
    } else {
      filter_ = fl;
      return true;
    }
  }

  bool setCalib(int32_t cl) {
    if (cl < 0 || cl > 8) {
      return false;
    } else {
      calib_ = cl;
      return true;
    }
  }

  ECPDOEntry<int32_t> force_x_pdo_;
  ECPDOEntry<int32_t> force_y_pdo_;
  ECPDOEntry<int32_t> force_z_pdo_;
  ECPDOEntry<int32_t> torque_x_pdo_;
  ECPDOEntry<int32_t> torque_y_pdo_;
  ECPDOEntry<int32_t> torque_z_pdo_;

  ECPDOEntry<int32_t> control1_pdo_;


  RTT::OutputPort<geometry_msgs::Wrench> wrench_port_;

  bool bias_;

  int32_t calib_;
  int32_t filter_;

};

char ati_name[] = "ati_driver";

typedef ECDriverFactoryService<ati_name, ATIDriver> ATIDriverFactory;

ORO_SERVICE_NAMED_PLUGIN(ATIDriverFactory, "ati_driver");
