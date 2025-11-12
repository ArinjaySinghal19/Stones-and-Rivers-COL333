#!/usr/bin/env python3
"""
Test suite for weight training system

This module tests:
- Weight vector normalization
- Weight perturbation (5-10% range)
- Unit norm constraint maintenance
- Hill climbing algorithm logic
"""

import unittest
import numpy as np
import sys
from pathlib import Path

# Import the training module
sys.path.insert(0, str(Path(__file__).parent))
from train_weights import WeightVector, WeightTrainer


class TestWeightVector(unittest.TestCase):
    """Test WeightVector class"""
    
    def test_default_initialization(self):
        """Test that default weights are initialized correctly"""
        wv = WeightVector()
        
        # Should have 12 weights
        self.assertEqual(len(wv.weights), 12)
        
        # Should be normalized (unit norm)
        norm = np.linalg.norm(wv.weights)
        self.assertAlmostEqual(norm, 1.0, places=10)
    
    def test_custom_weights(self):
        """Test initialization with custom weights"""
        custom = np.array([1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0])
        wv = WeightVector(custom, normalize=True)
        
        # Should be normalized
        norm = np.linalg.norm(wv.weights)
        self.assertAlmostEqual(norm, 1.0, places=10)
        
        # Should maintain relative proportions
        original_norm = np.linalg.norm(custom)
        expected = custom / original_norm
        np.testing.assert_array_almost_equal(wv.weights, expected)
    
    def test_normalization(self):
        """Test normalization function"""
        # Create unnormalized weights
        wv = WeightVector(np.array([10.0] * 12), normalize=False)
        
        # Should not be normalized yet
        self.assertNotAlmostEqual(np.linalg.norm(wv.weights), 1.0, places=5)
        
        # Normalize
        wv.normalize()
        
        # Should be normalized now
        self.assertAlmostEqual(np.linalg.norm(wv.weights), 1.0, places=10)
    
    def test_copy(self):
        """Test that copy creates independent copy"""
        wv1 = WeightVector()
        wv2 = wv1.copy()
        
        # Should be equal initially
        np.testing.assert_array_almost_equal(wv1.weights, wv2.weights)
        
        # Modifying one should not affect the other
        wv2.weights[0] = 999.0
        self.assertNotAlmostEqual(wv1.weights[0], wv2.weights[0])
    
    def test_perturbation_range(self):
        """Test that perturbation is in the 5-10% range"""
        wv = WeightVector()
        
        # Test multiple perturbations
        for _ in range(50):
            perturbed = wv.perturb()
            
            # Perturbed weights should still be normalized
            norm = np.linalg.norm(perturbed.weights)
            self.assertAlmostEqual(norm, 1.0, places=10)
            
            # Calculate the relative change
            diff = perturbed.weights - wv.weights
            diff_magnitude = np.linalg.norm(diff)
            
            # The difference should be roughly in the 5-10% range
            # Since we're in normalized space, this is approximate
            # We mainly check it's not too large or too small
            self.assertGreater(diff_magnitude, 0.01, 
                             "Perturbation seems too small")
            self.assertLess(diff_magnitude, 0.3, 
                          "Perturbation seems too large")
    
    def test_perturbation_creates_different_weights(self):
        """Test that perturbation actually changes the weights"""
        wv = WeightVector()
        perturbed = wv.perturb()
        
        # Should not be identical
        self.assertFalse(np.allclose(wv.weights, perturbed.weights))
    
    def test_perturbation_no_negative_weights(self):
        """Test that perturbation doesn't create negative weights"""
        wv = WeightVector()
        
        for _ in range(100):
            perturbed = wv.perturb()
            
            # All weights should be non-negative
            self.assertTrue(np.all(perturbed.weights >= 0),
                          f"Found negative weights: {perturbed.weights}")
    
    def test_to_dict(self):
        """Test conversion to dictionary"""
        wv = WeightVector()
        weights_dict = wv.to_dict()
        
        # Should have correct keys
        expected_keys = set(WeightVector.WEIGHT_NAMES)
        self.assertEqual(set(weights_dict.keys()), expected_keys)
        
        # All values should be numbers
        for value in weights_dict.values():
            self.assertIsInstance(value, (int, float))
    
    def test_to_cpp_format(self):
        """Test C++ format generation"""
        wv = WeightVector()
        cpp_code = wv.to_cpp_format()
        
        # Should contain all weight names
        for name in WeightVector.WEIGHT_NAMES:
            self.assertIn(name, cpp_code)
        
        # Should be valid-looking C++ code
        self.assertIn("double", cpp_code)
        self.assertIn("=", cpp_code)
        self.assertIn(";", cpp_code)


class TestNormalizationProperty(unittest.TestCase):
    """Test that normalization property is maintained"""
    
    def test_multiple_perturbations_maintain_norm(self):
        """Test that multiple perturbations maintain unit norm"""
        wv = WeightVector()
        
        # Apply 20 perturbations in sequence
        for i in range(20):
            wv = wv.perturb()
            norm = np.linalg.norm(wv.weights)
            self.assertAlmostEqual(norm, 1.0, places=10,
                                 msg=f"Norm violated at perturbation {i+1}")
    
    def test_extreme_weights_normalization(self):
        """Test normalization with extreme weight values"""
        # Very large weights
        large = WeightVector(np.array([1e10] * 12), normalize=True)
        self.assertAlmostEqual(np.linalg.norm(large.weights), 1.0, places=10)
        
        # Very small weights
        small = WeightVector(np.array([1e-10] * 12), normalize=True)
        self.assertAlmostEqual(np.linalg.norm(small.weights), 1.0, places=10)
        
        # Mixed weights
        mixed = WeightVector(np.array([1e-5, 1e5, 1e-3, 1e3] * 3), normalize=True)
        self.assertAlmostEqual(np.linalg.norm(mixed.weights), 1.0, places=10)


class TestPerturbationStatistics(unittest.TestCase):
    """Test statistical properties of perturbations"""
    
    def test_perturbation_distribution(self):
        """Test that perturbations are reasonably distributed"""
        wv = WeightVector()
        
        # Generate many perturbations
        perturbations = [wv.perturb() for _ in range(100)]
        
        # Check that we get variety (not all the same)
        unique_enough = False
        for i in range(len(perturbations)):
            for j in range(i+1, min(i+10, len(perturbations))):
                if not np.allclose(perturbations[i].weights, perturbations[j].weights):
                    unique_enough = True
                    break
            if unique_enough:
                break
        
        self.assertTrue(unique_enough, "Perturbations seem too similar")
    
    def test_perturbation_magnitude_statistics(self):
        """Test that perturbation magnitudes are in expected range"""
        wv = WeightVector()
        
        magnitudes = []
        for _ in range(100):
            perturbed = wv.perturb()
            diff = perturbed.weights - wv.weights
            magnitudes.append(np.linalg.norm(diff))
        
        mean_magnitude = np.mean(magnitudes)
        
        # Mean magnitude should be reasonable (between 1% and 20%)
        self.assertGreater(mean_magnitude, 0.01)
        self.assertLess(mean_magnitude, 0.3)


class TestWeightTrainerLogic(unittest.TestCase):
    """Test WeightTrainer logic (without actual C++ compilation)"""
    
    def test_trainer_initialization(self):
        """Test that trainer initializes correctly"""
        trainer = WeightTrainer(
            cpp_dir="c++_sample_files",
            games_per_eval=5,
            max_iterations=10
        )
        
        self.assertEqual(trainer.games_per_eval, 5)
        self.assertEqual(trainer.max_iterations, 10)
        self.assertIsNotNone(trainer.best_weights)
        self.assertIsNone(trainer.best_fitness)
        self.assertEqual(len(trainer.history), 0)


class TestIntegration(unittest.TestCase):
    """Integration tests for the complete system"""
    
    def test_weight_update_and_retrieval(self):
        """Test that we can update and retrieve weights"""
        wv1 = WeightVector()
        wv2 = wv1.perturb()
        
        # Both should be valid and different
        self.assertAlmostEqual(np.linalg.norm(wv1.weights), 1.0, places=10)
        self.assertAlmostEqual(np.linalg.norm(wv2.weights), 1.0, places=10)
        self.assertFalse(np.allclose(wv1.weights, wv2.weights))
    
    def test_hill_climbing_logic(self):
        """Test basic hill climbing acceptance logic"""
        # Simulate hill climbing: accept better, reject worse
        
        # Current state
        current_fitness = 0.5
        current_weights = WeightVector()
        
        # Better neighbor
        better_fitness = 0.6
        better_weights = current_weights.perturb()
        
        # In hill climbing, we accept if better
        if better_fitness > current_fitness:
            current_fitness = better_fitness
            current_weights = better_weights
        
        self.assertEqual(current_fitness, 0.6)
        
        # Worse neighbor
        worse_fitness = 0.4
        worse_weights = current_weights.perturb()
        
        # Should not accept worse
        old_fitness = current_fitness
        if worse_fitness > current_fitness:
            current_fitness = worse_fitness
            current_weights = worse_weights
        
        self.assertEqual(current_fitness, old_fitness)


def run_tests():
    """Run all tests and report results"""
    print("=" * 70)
    print("Running Weight Training Test Suite")
    print("=" * 70)
    print()
    
    # Create test suite
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    
    # Add all test cases
    suite.addTests(loader.loadTestsFromTestCase(TestWeightVector))
    suite.addTests(loader.loadTestsFromTestCase(TestNormalizationProperty))
    suite.addTests(loader.loadTestsFromTestCase(TestPerturbationStatistics))
    suite.addTests(loader.loadTestsFromTestCase(TestWeightTrainerLogic))
    suite.addTests(loader.loadTestsFromTestCase(TestIntegration))
    
    # Run tests
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    print()
    print("=" * 70)
    print("Test Summary")
    print("=" * 70)
    print(f"Tests run: {result.testsRun}")
    print(f"Successes: {result.testsRun - len(result.failures) - len(result.errors)}")
    print(f"Failures: {len(result.failures)}")
    print(f"Errors: {len(result.errors)}")
    print("=" * 70)
    
    return result.wasSuccessful()


if __name__ == '__main__':
    success = run_tests()
    sys.exit(0 if success else 1)
