/**
 * @author  Hauke Strasdat
 *
 * Copyright (C) 2010  Hauke Strasdat
 *                     Imperial College London
 *
 * transformations.h is part of RobotVision.
 *
 * RobotVision is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or any later version.
 *
 * RobotVision is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RV_TRANSFORMATIONS_H
#define RV_TRANSFORMATIONS_H

#include "TooN/se2.h"

#include "Camera/abstract_camera.h"
#include "Camera/linear_camera.h"
#include "maths_utils.h"
#include "sim3.h"


namespace RobotVision
{

  /** point transformation function using 3D ridig transformation SE3 */
  template <class A> inline TooN::Vector<3>
      transform(const TooN::SE3<A>& T, const TooN::Vector<3,A>& x)
  {
    return T.get_rotation()*x + T.get_translation();
  }

  /** point transformation function using 3D similarity transformation
      * Sim3 */
  template <class A> inline TooN::Vector<3>
      transform(const RobotVision::Sim3<A>& T, const TooN::Vector<3,A>& x)
  {
    return T.get_scale()*(T.get_rotation()*x) + T.get_translation();
  }


  /**
     * Abstract prediction class
     * Frame: How is the frame/pose represented? (e.g. SE3)
     * FrameDoF: How many DoF has the pose/frame? (e.g. 6 DoF, that is
     *           3 DoF translation, 3 DoF rotation)
     * PointParNum: number of parameters to represent a point
     *              (4 for a 3D homogenious point)
     * PointDoF: DoF of a point (3 DoF for a 3D homogenious point)
     * ObsDim: dimensions of observation (2 dim for (u,v) image
     *         measurement)
     */
  template <typename Frame,
  int FrameDoF,
  int PointParNum,
  int PointDoF,
  int ObsDim>
  class AbstractPrediction
  {
  public:

    /** Map a world point x into the camera/sensor coordinate frame T
          * and create an observation*/
    virtual TooN::Vector<ObsDim>
        map(const Frame & T,
            const TooN::Vector<PointParNum> & x) const = 0;

    /** Jacobian wrt. frame: use numerical Jacobian as default */
    virtual TooN::Matrix<ObsDim,FrameDoF>
        frameJac(const Frame & T,
                 const TooN::Vector<PointParNum> & x) const
    {
      double h = 0.000000000001;
      TooN::Matrix<ObsDim,FrameDoF> J_pose  = TooN::Zeros;

      TooN::Vector<ObsDim>  fun = map(T,x);
      for (uint i=0; i<FrameDoF; ++i)
      {
        TooN::Vector<FrameDoF> eps = TooN::Zeros;
        eps[i] = h;

        J_pose.T()[i] = (map(add(T,eps),x) -fun)/h ;
      }
      return J_pose;
    }

    /** Jacobian wrt. point: use numerical Jacobian as default */
    virtual TooN::Matrix<ObsDim,PointDoF>
        pointJac(const Frame & T,
                 const TooN::Vector<PointParNum> & x) const
    {
      double h = 0.000000000001;
      TooN::Matrix<ObsDim,PointDoF> J_x  = TooN::Zeros;
      TooN::Vector<ObsDim> fun = map(T,x);
      for (uint i=0; i<PointDoF; ++i)
      {
        TooN::Vector<PointDoF> eps = TooN::Zeros;
        eps[i] = h;

        J_x.T()[i] = (map(T,add(x,eps)) -fun)/h ;

      }
      return J_x;
    }

    /** Add an incermental update delta to pose/frame T*/
    virtual Frame
        add(const Frame & T,
            const TooN::Vector<FrameDoF> & delta) const = 0;

    /** Add an incremental update delta to point x*/
    virtual TooN::Vector<PointParNum>
        add(const TooN::Vector<PointParNum> & x,
            const TooN::Vector<PointDoF> & delta) const = 0;

    virtual int first_rot_id() const = 0;
    virtual int num_rot_pars() const = 0;
    virtual int first_trans_id() const = 0;
    virtual int num_trans_pars() const = 0;
  };


  /** abstract prediction class dependig on
      * 3D rigid body transformations SE3 */
  template <int PointParNum, int PointDoF, int ObsDim>
      class SE3_AbstractPoint
        : public AbstractPrediction<TooN::SE3<>,6,PointParNum,PointDoF,ObsDim>
  {
    TooN::SE3<> add(const TooN::SE3<> &T, const TooN::Vector<6> & delta) const
    {
      return TooN::SE3<>(delta)*T;
    }

    inline int first_rot_id() const
    {
      return 3;
    }
    inline int num_rot_pars() const
    {
      return 3;
    }
    inline int first_trans_id() const
    {
      return 0;
    }
    inline int num_trans_pars() const
    {
      return 3;
    }

  };


  /** abstract prediction class dependig on
      * 2D rigid body transformations SE2 */
  template <int PointParNum, int PointDoF, int ObsDim>
      class SE2_AbstractPoint
        : public AbstractPrediction<TooN::SE2<>,3,PointParNum,PointDoF,ObsDim>
  {
    TooN::SE2<> add(const TooN::SE2<> &T, const TooN::Vector<3> & delta) const
    {
      return TooN::SE2<>(delta)*T;
    }

    inline int first_rot_id() const
    {
      return 2;
    }
    inline int num_rot_pars() const
    {
      return 1;
    }
    inline int first_trans_id() const
    {
      return 0;
    }
    inline int num_trans_pars() const
    {
      return 2;
    }

  };




  /** 2D bearing-only prediction class */
  class SE2XY: public SE2_AbstractPoint<2, 2, 1>
  {
  public:
    SE2XY()
    {
    }

    inline TooN::Vector<1> map(const TooN::SE2<> & T,
                               const TooN::Vector<2>& x) const
    {
      return TooN::project(T.get_rotation()*x + T.get_translation() );
    }

    TooN::Vector<2> add(const TooN::Vector<2> & p,
                        const TooN::Vector<2> & delta) const
    {
      return p+delta;
    }
  };

  /** 3D Euclidean point class */
  class SE3XYZ: public SE3_AbstractPoint<3, 3, 2>{
  public:

    SE3XYZ()
    {
    }

    SE3XYZ(const RobotVision::LinearCamera & cam_pars)
    {
      this->cam_pars = cam_pars;
    }

    inline TooN::Vector<2> map(const TooN::SE3<> & T,
                               const TooN::Vector<3>& x) const
    {
      return cam_pars.map(project(transform(T,x)));
    }


    TooN::Matrix<2,6> frameJac(const TooN::SE3<> & T,
                               const TooN::Vector<3> & xyz) const
    {
      TooN::Matrix<2,6> J_frame;

      /** Following Ethan Eade's Phd thesis */
      TooN::Vector<3> xyz_trans = T.get_rotation()*xyz + T.get_translation();

      double x = xyz_trans[0];
      double y = xyz_trans[1];
      double z = xyz_trans[2];
      double z_2 = Po2(z);

      J_frame[0]
          = TooN::makeVector(1./z, 0, -x/z_2, -x*y/z_2, 1+(Po2(x)/z_2), -y/z);
      J_frame[1]
          = TooN::makeVector(0, 1./z, -y/z_2, -(1+Po2(y)/z_2), x*y/z_2, x/z);

      return cam_pars.jacobian() * J_frame;

    }

    TooN::Matrix<2,3> pointJac(const TooN::SE3<> & T,
                               const TooN::Vector<3> & xyz) const
    {

      /** Following Ethan Eade's Phd thesis */
      TooN::Vector<3> xyz_trans = T.get_rotation()*xyz + T.get_translation();

      double x = xyz_trans[0];
      double y = xyz_trans[1];
      double z = xyz_trans[2];

      TooN::Matrix<2,3> tmp;
      tmp[0] = TooN::makeVector(1,0,-x/z);
      tmp[1] = TooN::makeVector(0,1,-y/z);

      TooN::Matrix<2,3> J_x = 1./z * tmp * T.get_rotation();

      return cam_pars.jacobian() * J_x;
    }

    TooN::Vector<3> add(const TooN::Vector<3> & x,
                        const TooN::Vector<3> & delta) const
    {
      return x+delta;
    }

  private:
    LinearCamera  cam_pars;

  };


  /** 3D inverse depth point class*/
  class SE3UVQ : public SE3_AbstractPoint<3, 3, 2>{
  public:

    SE3UVQ (const LinearCamera & cam_pars)

    {
      this->cam_pars = cam_pars;
    }

    inline TooN::Vector<2> map(const TooN::SE3<> & T,
                               const TooN::Vector<3>& uvq) const
    {

      TooN::Vector<3> x = 1./uvq[2]*TooN::makeVector(uvq[0],uvq[1],1);

      return cam_pars.map(project(T.get_rotation()*x + T.get_translation()));
    }


    TooN::Matrix<2,6> frameJac(const TooN::SE3<> & T,
                               const TooN::Vector<3>& uvq) const
    {
      TooN::Matrix<2,6> J_frame;
      TooN::Vector<3> xyz = 1./uvq[2]*TooN::makeVector(uvq[0],uvq[1],1);

      /** Following Ethan Eade's Phd thesis */
      TooN::Vector<3> xyz_trans = T.get_rotation()*xyz + T.get_translation();

      double x = xyz_trans[0];
      double y = xyz_trans[1];
      double z = xyz_trans[2];
      double z_2 = Po2(z);

      J_frame[0]
          = TooN::makeVector(1./z, 0, -x/z_2, -x*y/z_2, 1+(Po2(x)/z_2), -y/z);
      J_frame[1]
          = TooN::makeVector(0, 1./z, -y/z_2, -(1+Po2(y)/z_2), x*y/z_2, x/z);

      return cam_pars.jacobian() * J_frame;

    }

    TooN::Matrix<2,3> pointJac(const TooN::SE3<> & T,
                               const TooN::Vector<3>& uvq) const
    {
      TooN::Vector<3> xyz = 1./uvq[2]*TooN::makeVector(uvq[0],uvq[1],1);

      const TooN::Matrix<3,3> & R = T.get_rotation().get_matrix();

      /** Following Ethan Eade's Phd thesis */
      TooN::Vector<3> xyz_trans = R*xyz + T.get_translation();

      double x = xyz_trans[0];
      double y = xyz_trans[1];
      double z = xyz_trans[2];

      TooN::Matrix<3,3> R12t;
      R12t.T()[0] = R.T()[0];
      R12t.T()[1] = R.T()[1];
      R12t.T()[2] = T.get_translation();

      TooN::Matrix<2,3> tmp;
      tmp[0] = TooN::makeVector(1,0,-x/z);
      tmp[1] = TooN::makeVector(0,1,-y/z);

      TooN::Matrix<2,3> J_x = 1./(z*uvq[2]) * tmp * R12t;

      return cam_pars.jacobian() * J_x;
    }


    TooN::Vector<3> add(const TooN::Vector<3> & x,
                        const TooN::Vector<3> & delta) const
    {
      return x+delta;
    }

  private:
    LinearCamera  cam_pars;
  };


  /** observation class */
  template <int ObsDim>
      class IdObs{
      public:
    IdObs(){}
    IdObs(int point_id, int frame_id, const TooN::Vector<ObsDim> & obs)
      : frame_id(frame_id), point_id(point_id), obs(obs)
    {
    }

    int frame_id;
    int point_id;
    TooN::Vector<ObsDim> obs;
  };


  /** observation class with inverse uncertainty*/
  template <int ObsDim>
      class IdObsLambda : public IdObs<ObsDim>{
      public:
    IdObsLambda(){}
    IdObsLambda(int point_id,
                int frame_id,
                const TooN::Vector<ObsDim> & obs,
                const TooN::Matrix<ObsDim,ObsDim> & lambda)
                  : IdObs<ObsDim>(point_id, frame_id,  obs) , lambda(lambda)
    {
    }
    TooN::Matrix<2,2> lambda;
  };


  /** Abstract class for relative pose constraints between
      * two obsolute pose transformations
      * Trans: type of pose transformation (Se2, Se3, Sim3,...)
      * TransDoF: DoF of transformation
      */
  template <typename Trans, int TransDoF>
      class AbstractConFun{
      public:

    /** difference function betwen two absolute transformations T1, T2
      * and a relatibe contraint C (residual in optimisation)
      */
    virtual TooN::Vector<TransDoF> diff(const Trans & T1,
                                        const Trans& C,
                                        const Trans & T2) const = 0;

    /** Jacobian wrt. to the first constraint T1
      * use nummerical Jacobian as default
      */
    virtual TooN::Matrix<TransDoF,TransDoF> d_diff_dT1(const Trans & T1,
                                                       const Trans& C,
                                                       const Trans & T2)const
    {
      double h = 0.000000000001;
      TooN::Matrix<TransDoF> J  = TooN::Zeros;
      TooN::Vector<TransDoF> fun = diff(T1,C,T2);

      for (uint i=0; i<TransDoF; ++i)
      {
        TooN::Vector<TransDoF> eps = TooN::Zeros;
        eps[i] = h;
        J.T()[i] = (diff(add(T1,eps),C,T2) -fun)/h ;
      }
      return J;
    }

    /** Jacobian wrt. to the second t constraint T2
          * use nummerical Jacobian as default
          */
    virtual TooN::Matrix<TransDoF,TransDoF> d_diff_dT2(const Trans & T1,
                                                       const Trans& C,
                                                       const Trans & T2)const
    {
      double h = 0.000000000001;
      TooN::Matrix<TransDoF> J  = TooN::Zeros;
      TooN::Vector<TransDoF> fun = diff(T1,C,T2);

      for (uint i=0; i<TransDoF; ++i)
      {
        TooN::Vector<TransDoF> eps = TooN::Zeros;
        eps[i] = h;
        J.T()[i] = (diff(T1,C,add(T2,eps)) -fun)/h ;
      }
      return J;
    }

    /** Incremental update delta of transformation T1 */
    virtual Trans add(const Trans & T1,
                      const TooN::Vector<TransDoF> & delta)const = 0;
  };


  namespace SE3Helper
  {

    /** logarithic map of 3D rotation group So3 */
    TooN::Vector<3> ln_so3(const TooN::Matrix<3> & R)
    {
      double d = 0.5*(R(0,0)+R(1,1)+R(2,2)-1);
      TooN::Vector<3> omega;
      if (d>0.99999)
      {
        omega=0.5*deltaR(R);
      }
      else
      {
        double theta = acos(d);
        omega = theta/(2*sqrt(1-d*d))*deltaR(R);
      }
      return omega;
    }

    /** logarithic map of 3D pseudo rigid transformation group <So3,R3>*/
    TooN::Vector<6> ln_so3xR3(const TooN::SE3<> & T)
    {
      TooN::Vector<6> res;
      res.slice<0,3>() = ln_so3(T.get_rotation().get_matrix());
      res.slice<3,3>() = T.get_translation();
      return res;
    }

    /** logarithic map of 3D rigid transformation group Se3*/
    TooN::Vector<6> ln(const TooN::Matrix<3> &R, const TooN::Vector<3> & t)
    {
      TooN::Vector<6> v;
      TooN::Vector <3> omega;
      TooN::Matrix <3> Omega;
      TooN::Matrix <3> V_inv;

      double d = 0.5*( R(0,0)+R(1,1)+R(2,2)-1);

      if (d>0.99999)
      {
        omega = 0.5*deltaR(R);
        Omega = skew(omega);
        V_inv = TooN::Identity(3)- 0.5*Omega + (1./12.)*(Omega*Omega);
      }
      else
      {
        double theta = acos(d);
        omega = theta/(2*sqrt(1-d*d))*deltaR(R);
        Omega = skew(omega);
        V_inv
            = TooN::Identity(3)
              - 0.5*Omega
              + (1-theta/(2*tan(theta/2)))/(theta*theta)*(Omega*Omega);
      }
      v.slice<0,3>() = omega;
      v.slice<3,3>() = V_inv*t;
      return v;
    }

    TooN::Matrix<3,9> M3x9(TooN::Vector<3> & a, TooN::Matrix<3> & B)
    {
      TooN::Matrix<3,9> J;
      J.T()[0] = a;
      J.T()[1] = -B.T()[2];
      J.T()[2] = B.T()[1];
      J.T()[3] = B.T()[2];
      J.T()[4] = a;
      J.T()[5] = -B.T()[0];
      J.T()[6] = -B.T()[1];
      J.T()[7] = B.T()[0];
      J.T()[8] = a;
      return J;
    }

    TooN::Matrix<3,9> dlnR_dR(const TooN::Matrix<3,3> & R)
    {


      double d = 0.5*(R(0,0)+R(1,1)+R(2,2)-1);

      TooN::Vector<3> a ;
      TooN::Matrix<3,3> B;
      if(d>0.99999)
      {
        a = TooN::makeVector(0,0,0);
        B = -0.5*TooN::Identity;
      }
      else
      {
        double theta = acos(d);
        double d2 = d*d;
        double sq = sqrt(1-d2);
        a = (d*theta-sq)/(4*Po3(sq))*deltaR(R);
        B = -theta/(2*sq)*TooN::Identity(3);
      }
      return M3x9(a,B);
    }

    TooN::Matrix<3> ddeltaRt_dR(const TooN::SE3<> & T)
    {
      TooN::Matrix<3> J;
      TooN::Matrix<3> R = T.get_rotation().get_matrix();
      TooN::Vector<3> t = T.get_translation();
      TooN::Vector<3> abc = deltaR(R);
      double a = abc[0];
      double b = abc[1];
      double c = abc[2];

      J[0] = TooN::makeVector(-b*t[1]-c*t[2], 2*b*t[0]-a*t[1], 2*c*t[0]-a*t[2]);
      J[1] = TooN::makeVector(-b*t[0]+2*a*t[1],-a*t[0]-c*t[2], 2*c*t[1]-b*t[2]);
      J[2] = TooN::makeVector(-c*t[0]+2*a*t[2],-c*t[1]+2*b*t[2],-a*t[0]-b*t[1]);

      return J;
    }

    TooN::Matrix<3,9> dVinvt_dR(const TooN::SE3<> & T)
    {
      TooN::Vector<3> a;
      TooN::Matrix<3,3> B;

      TooN::Matrix<3> R = T.get_rotation().get_matrix();
      TooN::Vector<3> t = T.get_translation();

      double d = 0.5*( R(0,0)+R(1,1)+R(2,2)-1);

      if (d>0.9999)
      {
        a = TooN::makeVector(0,0,0);
        B = TooN::Zeros;
      }
      else
      {
        double theta = acos(d);
        double theta2 = theta*theta;
        double oned2 = (1-d*d);
        double sq = sqrt(oned2);
        double cot = 1./tan(0.5*theta);
        double csc2 = Po2(1./sin(0.5*theta));

        TooN::Matrix<3> skewR = skew(deltaR(R));
        a = -(d*theta-sq)/(8*Po3(sq))*skewR*t
            + (((theta*sq-d*theta2)*(0.5*theta*cot-1))
               -theta*sq*((0.25*theta*cot)+0.125*theta2*csc2-1))
            /(4*theta2*Po2(oned2))*(skewR*skewR*t);
        B = -0.5*theta/(2*sq)*skew(t)
            - (theta*cot-2)/(8*oned2) * ddeltaRt_dR(T);
      }
      return M3x9(a,B);
    }

    /** Jacobian of SE3 logarithmic map wrt. T */
    TooN::Matrix<6,12> dlnT_dT(const TooN::SE3<> & T)
    {
      TooN::Matrix<6,12> J = TooN::Zeros;
      J.slice<0,0,3,9>() = dlnR_dR(T.get_rotation().get_matrix());
      J.slice<3,0,3,9>() = dVinvt_dR(T);

      TooN::Matrix<3> R = T.get_rotation().get_matrix();
      TooN::Vector <3> omega;
      TooN::Matrix <3> Omega;
      TooN::Matrix <3> V_inv;
      double d = 0.5*( R(0,0)+R(1,1)+R(2,2)-1);
      if (d>0.99999)
      {
        omega = 0.5*deltaR(R);
        Omega = skew(omega);
        V_inv = TooN::Identity(3)- 0.5*Omega + (1./12.)*(Omega*Omega);
      }
      else
      {
        double theta = acos(d);
        omega = theta/(2*sqrt(1-d*d))*deltaR(R);
        Omega = skew(omega);
        V_inv = TooN::Identity(3)
                - 0.5*Omega
                + (1-theta/(2*tan(theta/2)))/(theta*theta)*(Omega*Omega);
      }
      J.slice<3,9,3,3>() = V_inv;
      return J;
    }

    /** Jacobain of incremenal update 'exp(delta)T' wrt. delta*/
    TooN::Matrix<12,6> dexp_x_T_ddelta(const TooN::SE3<> & T)
    {
      TooN::Matrix<12,6> J = TooN::Zeros;
      TooN::Matrix<3,3> R = T.get_rotation().get_matrix();
      TooN::Vector<3> t = T.get_translation();
      J.slice<0,3,3,3>() = -skew(R.T()[0]);
      J.slice<3,3,3,3>() = -skew(R.T()[1]);
      J.slice<6,3,3,3>() = -skew(R.T()[2]);
      J.slice<9,3,3,3>() = -skew(t);
      J.slice<9,0,3,3>() = TooN::Identity;

      return J;
    }

    /** Jacobain of 'diff' wrt. first transformation T1 */
    TooN::Matrix<12,12> dDiff_dT1(const TooN::SE3<> & Tc,
                                  const TooN::SE3<> & T2)
    {
      TooN::Matrix<12,12> J = TooN::Zeros;
      TooN::Matrix<3,3> R2 = T2.get_rotation().get_matrix();
      TooN::Matrix<3,3> Rc = Tc.get_rotation().get_matrix();
      TooN::Vector<3> t2 = T2.get_translation();
      J.slice<0,0,9,9>() = kron(R2,Rc);
      J.slice<9,0,3,9>() = kron(-(R2.T()*t2.as_col()).T(),Rc);
      J.slice<9,9,3,3>() = Rc;

      return J;
    }

    /** Jacobain of 'diff' wrt. second transformation T2 */
    TooN::Matrix<12,12> dDiff_dT2(const TooN::SE3<> & T1,
                                  const TooN::SE3<> & Tc,
                                  const TooN::SE3<> & T2)
    {
      TooN::Matrix<12,12> J = TooN::Zeros;
      TooN::Matrix<3,3> R = T1.get_rotation().get_matrix();
      TooN::Matrix<3,3> R2 = T2.get_rotation().get_matrix();
      TooN::Matrix<3,3> Rc = Tc.get_rotation().get_matrix();
      TooN::Vector<3> t2 = T2.get_translation();
      TooN::Matrix<3> I = TooN::Identity;

      J.slice<0,0,9,3>() = kron(I,Rc*R.T()[0].as_col());
      J.slice<0,3,9,3>() = kron(I,Rc*R.T()[1].as_col());
      J.slice<0,6,9,3>() = kron(I,Rc*R.T()[2].as_col());

      J.slice<9,0,3,3>() = kron(-t2.as_row(),Rc*R.T()[0].as_col());
      J.slice<9,3,3,3>() = kron(-t2.as_row(),Rc*R.T()[1].as_col());
      J.slice<9,6,3,3>() = kron(-t2.as_row(),Rc*R.T()[2].as_col());

      J.slice<9,9,3,3>() = -Rc*R*R2.T();

      return J;
    }
  }


  /** class for ridig transformation SE3 constraints */
  class SE3ConFun : public AbstractConFun<TooN::SE3<>,6>
  {
  private:

  public:
    TooN::Vector<6> diff(const TooN::SE3<> & T1,
                         const TooN::SE3<>& C,
                         const TooN::SE3<> & T2)const
    {
      TooN::SE3<> D = (C*T1) * T2.inverse();
      return SE3Helper::ln(D.get_rotation().get_matrix(),D.get_translation());
    }


    TooN::Matrix<6,6> d_diff_dT1(const TooN::SE3<> & T1,
                                 const TooN::SE3<>& C,
                                 const TooN::SE3<> & T2)const
    {
      TooN::Matrix<12,6> dT1_dlnT1 = SE3Helper::dexp_x_T_ddelta(T1);

      TooN::Matrix<12,12> dD_dT1 = SE3Helper::dDiff_dT1(C,T2);

      TooN::SE3<> D = T1*C*T2.inverse();
      TooN::Matrix<6,12> dlnD_dD = SE3Helper::dlnT_dT(D);

      return dlnD_dD*dD_dT1*dT1_dlnT1 ;
    }

    TooN::Matrix<6,6> d_diff_dT2(const TooN::SE3<> & T1,
                                 const TooN::SE3<>& C,
                                 const TooN::SE3<> & T2)const
    {
      TooN::Matrix<12,6> dT2_dlnT2 = SE3Helper::dexp_x_T_ddelta(T2);

      TooN::Matrix<12,12> dD_dT2 = SE3Helper::dDiff_dT2(T1,C,T2);

      TooN::SE3<> D = T1*C*T2.inverse();
      TooN::Matrix<6,12> dlnD_dD = SE3Helper::dlnT_dT(D);
      return dlnD_dD*dD_dT2*dT2_dlnT2;
    }

    TooN::SE3<> add(const TooN::SE3<> & T, const TooN::Vector<6> & delta)const
    {
      return TooN::SE3<>(delta) * T;
    }
  };


  /** class for pseudo rigid transformation <So3,R3> constraints*/
  class SO3xR3ConFun : public AbstractConFun<TooN::SE3<>,6>
  {
  private:


  public:
    TooN::Vector<6> diff(const TooN::SE3<> & T1,
                         const TooN::SE3<>& C,
                         const TooN::SE3<> & T2)const
    {
      return SE3Helper::ln_so3xR3((C*T1) * T2.inverse());
    }

    TooN::SE3<> add(const TooN::SE3<> & T, const TooN::Vector<6> & delta)const
    {

      TooN::Vector<3> omega = delta.slice(3,3);
      TooN::SO3<> R = TooN::SO3<>(omega) * T.get_rotation();

      return TooN::SE3<>(R, T.get_translation()+delta.slice(0,3));
    }
  };



  /** class for ridig transformation Se3 constraints
      * using <So3,R3> as residual*/
  class SE3ConFunSO3xR3 : public AbstractConFun<TooN::SE3<>,6>
  {
  private:


  public:
    TooN::Vector<6> diff(const TooN::SE3<> & T1,
                         const TooN::SE3<>& C,
                         const TooN::SE3<> & T2)const
    {
      return SE3Helper::ln_so3xR3((C*T1) * T2.inverse());
    }

    TooN::SE3<> add(const TooN::SE3<> & T, const TooN::Vector<6> & delta)const
    {
      return  TooN::SE3<>(delta) * T;
    }
  };


  /** class for similarity transformation Sim3 constraints */
  class Sim3ConFun : public AbstractConFun<RobotVision::Sim3<>,7>
  {
  public:
    TooN::Vector<7> diff(const RobotVision::Sim3<> & T1,
                         const RobotVision::Sim3<>& C,
                         const RobotVision::Sim3<> & T2)const
    {
      return ((C*T1) * T2.inverse()).ln();
    }


    RobotVision::Sim3<> add(const RobotVision::Sim3<> & T,
                            const TooN::Vector<7> & delta)const
    {

      return RobotVision::Sim3<>(delta) * T;
    }
  };


}


#endif // RV_TRANSFORMATIONS_H
