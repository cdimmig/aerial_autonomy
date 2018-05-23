#include <aerial_autonomy/types/discrete_reference_trajectory_interpolate.h>
#include <gtest/gtest.h>

TEST(DiscreteReferenceTrajectoryInterpolate, TimeTooSmall) {
  DiscreteReferenceTrajectoryInterpolate<double, double> ref;
  ref.ts = {0, 1, 2, 3, 4, 5};
  ref.states = {0, 1, 2, 3, 4, 5};
  ref.controls = {0, 1, 2, 3, 4, 5};

  ASSERT_THROW(ref.atTime(-1), std::out_of_range);
}

TEST(DiscreteReferenceTrajectoryInterpolate, TimeTooLarge) {
  DiscreteReferenceTrajectoryInterpolate<double, double> ref;
  ref.ts = {0, 1, 2, 3, 4, 5};
  ref.states = {0, 1, 2, 3, 4, 5};
  ref.controls = {0, 1, 2, 3, 4, 5};

  ASSERT_THROW(ref.atTime(5.1), std::out_of_range);
}

TEST(DiscreteReferenceTrajectoryInterpolate, TimeInMiddle) {
  DiscreteReferenceTrajectoryInterpolate<double, double> ref;
  ref.ts = {0, 1, 2, 3, 4, 5};
  ref.states = {0, 1, 2, 3, 4, 5};
  ref.controls = {0, 1, 2, 3, 4, 5};

  auto ref_t = ref.atTime(4.5);
  ASSERT_NEAR(std::get<0>(ref_t), 4.5, 1e-6);
  ASSERT_NEAR(std::get<1>(ref_t), 4.5, 1e-6);

  ref_t = ref.atTime(3.1);
  ASSERT_NEAR(std::get<0>(ref_t), 3.1, 1e-6);
  ASSERT_NEAR(std::get<1>(ref_t), 3.1, 1e-6);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
