#include <gtest/gtest.h>
#include "phantom_vault/storage.hpp"
#include "phantom_vault/encryption.hpp"
#include <vector>
#include <string>
#include <chrono>

using namespace phantom_vault::storage;
using namespace std::chrono;

class RecoveryTest : public ::testing::Test {
protected:
    void SetUp() override {
        storage = std::make_unique<SecureStorage>();
        
        // Initialize with a test master key
        std::vector<uint8_t> master_key(32, 0x42);
        ASSERT_TRUE(storage->initialize(master_key));
    }

    void TearDown() override {
        storage.reset();
    }

    std::unique_ptr<SecureStorage> storage;
};

TEST_F(RecoveryTest, SetupPasswordRecovery) {
    // Create test recovery info
    RecoveryInfo recovery_info;
    recovery_info.vault_id = "test-vault-1";
    recovery_info.attempts_remaining = 3;
    recovery_info.created_time = system_clock::now();
    recovery_info.last_used = system_clock::now();
    recovery_info.recovery_key = {0x01, 0x02, 0x03, 0x04};
    recovery_info.recovery_iv = {0x05, 0x06, 0x07, 0x08};

    // Add recovery questions
    RecoveryQuestion question1;
    question1.question_id = "q1";
    question1.question_text = "What is your favorite color?";
    question1.answer_hash = {0x11, 0x12, 0x13, 0x14};
    question1.salt = {0x21, 0x22, 0x23, 0x24};
    recovery_info.questions.push_back(question1);

    RecoveryQuestion question2;
    question2.question_id = "q2";
    question2.question_text = "What was your first pet's name?";
    question2.answer_hash = {0x31, 0x32, 0x33, 0x34};
    question2.salt = {0x41, 0x42, 0x43, 0x44};
    recovery_info.questions.push_back(question2);

    // Test setup
    EXPECT_TRUE(storage->setupPasswordRecovery("test-vault-1", recovery_info));
    EXPECT_TRUE(storage->hasPasswordRecovery("test-vault-1"));
}

TEST_F(RecoveryTest, GetRecoveryQuestions) {
    // First setup recovery
    RecoveryInfo recovery_info;
    recovery_info.vault_id = "test-vault-2";
    recovery_info.attempts_remaining = 3;
    recovery_info.created_time = system_clock::now();
    recovery_info.last_used = system_clock::now();
    recovery_info.recovery_key = {0x01, 0x02, 0x03, 0x04};
    recovery_info.recovery_iv = {0x05, 0x06, 0x07, 0x08};

    RecoveryQuestion question;
    question.question_id = "q1";
    question.question_text = "What is your mother's maiden name?";
    question.answer_hash = {0x11, 0x12, 0x13, 0x14};
    question.salt = {0x21, 0x22, 0x23, 0x24};
    recovery_info.questions.push_back(question);

    ASSERT_TRUE(storage->setupPasswordRecovery("test-vault-2", recovery_info));

    // Test getting questions
    auto questions = storage->getRecoveryQuestions("test-vault-2");
    EXPECT_EQ(questions.size(), 1);
    EXPECT_EQ(questions[0].question_id, "q1");
    EXPECT_EQ(questions[0].question_text, "What is your mother's maiden name?");
}

TEST_F(RecoveryTest, VerifyRecoveryAnswers) {
    // Setup recovery with known answers
    RecoveryInfo recovery_info;
    recovery_info.vault_id = "test-vault-3";
    recovery_info.attempts_remaining = 3;
    recovery_info.created_time = system_clock::now();
    recovery_info.last_used = system_clock::now();
    recovery_info.recovery_key = {0x01, 0x02, 0x03, 0x04};
    recovery_info.recovery_iv = {0x05, 0x06, 0x07, 0x08};

    // Create encryption engine to generate proper hashes
    phantom_vault::EncryptionEngine encryption;
    ASSERT_TRUE(encryption.initialize());

    RecoveryQuestion question;
    question.question_id = "q1";
    question.question_text = "What is your favorite color?";
    question.salt = encryption.generateSalt();
    std::string answer = "blue";
    question.answer_hash = encryption.deriveKeyFromPassword(answer, question.salt);
    recovery_info.questions.push_back(question);

    ASSERT_TRUE(storage->setupPasswordRecovery("test-vault-3", recovery_info));

    // Test correct answer
    std::vector<std::string> correct_answers = {"blue"};
    auto recovery_key = storage->verifyRecoveryAnswers("test-vault-3", correct_answers);
    EXPECT_FALSE(recovery_key.empty());

    // Test incorrect answer
    std::vector<std::string> incorrect_answers = {"red"};
    auto empty_key = storage->verifyRecoveryAnswers("test-vault-3", incorrect_answers);
    EXPECT_TRUE(empty_key.empty());
}

TEST_F(RecoveryTest, VerifyRecoveryAnswersAttemptsLimit) {
    // Setup recovery
    RecoveryInfo recovery_info;
    recovery_info.vault_id = "test-vault-4";
    recovery_info.attempts_remaining = 2; // Start with 2 attempts
    recovery_info.created_time = system_clock::now();
    recovery_info.last_used = system_clock::now();
    recovery_info.recovery_key = {0x01, 0x02, 0x03, 0x04};
    recovery_info.recovery_iv = {0x05, 0x06, 0x07, 0x08};

    phantom_vault::EncryptionEngine encryption;
    ASSERT_TRUE(encryption.initialize());

    RecoveryQuestion question;
    question.question_id = "q1";
    question.question_text = "What is your favorite color?";
    question.salt = encryption.generateSalt();
    std::string answer = "blue";
    question.answer_hash = encryption.deriveKeyFromPassword(answer, question.salt);
    recovery_info.questions.push_back(question);

    ASSERT_TRUE(storage->setupPasswordRecovery("test-vault-4", recovery_info));

    // First incorrect attempt
    std::vector<std::string> wrong_answers = {"red"};
    auto empty_key1 = storage->verifyRecoveryAnswers("test-vault-4", wrong_answers);
    EXPECT_TRUE(empty_key1.empty());

    // Second incorrect attempt
    auto empty_key2 = storage->verifyRecoveryAnswers("test-vault-4", wrong_answers);
    EXPECT_TRUE(empty_key2.empty());

    // Third attempt should fail due to no attempts remaining
    auto empty_key3 = storage->verifyRecoveryAnswers("test-vault-4", wrong_answers);
    EXPECT_TRUE(empty_key3.empty());
}

TEST_F(RecoveryTest, RemovePasswordRecovery) {
    // Setup recovery
    RecoveryInfo recovery_info;
    recovery_info.vault_id = "test-vault-5";
    recovery_info.attempts_remaining = 3;
    recovery_info.created_time = system_clock::now();
    recovery_info.last_used = system_clock::now();
    recovery_info.recovery_key = {0x01, 0x02, 0x03, 0x04};
    recovery_info.recovery_iv = {0x05, 0x06, 0x07, 0x08};

    RecoveryQuestion question;
    question.question_id = "q1";
    question.question_text = "What is your favorite color?";
    question.answer_hash = {0x11, 0x12, 0x13, 0x14};
    question.salt = {0x21, 0x22, 0x23, 0x24};
    recovery_info.questions.push_back(question);

    ASSERT_TRUE(storage->setupPasswordRecovery("test-vault-5", recovery_info));
    EXPECT_TRUE(storage->hasPasswordRecovery("test-vault-5"));

    // Remove recovery
    EXPECT_TRUE(storage->removePasswordRecovery("test-vault-5"));
    EXPECT_FALSE(storage->hasPasswordRecovery("test-vault-5"));
}

TEST_F(RecoveryTest, NonExistentVault) {
    // Test operations on non-existent vault
    EXPECT_FALSE(storage->hasPasswordRecovery("non-existent-vault"));
    
    auto questions = storage->getRecoveryQuestions("non-existent-vault");
    EXPECT_TRUE(questions.empty());
    
    std::vector<std::string> answers = {"test"};
    auto recovery_key = storage->verifyRecoveryAnswers("non-existent-vault", answers);
    EXPECT_TRUE(recovery_key.empty());
}

TEST_F(RecoveryTest, WrongNumberOfAnswers) {
    // Setup recovery with 2 questions
    RecoveryInfo recovery_info;
    recovery_info.vault_id = "test-vault-6";
    recovery_info.attempts_remaining = 3;
    recovery_info.created_time = system_clock::now();
    recovery_info.last_used = system_clock::now();
    recovery_info.recovery_key = {0x01, 0x02, 0x03, 0x04};
    recovery_info.recovery_iv = {0x05, 0x06, 0x07, 0x08};

    phantom_vault::EncryptionEngine encryption;
    ASSERT_TRUE(encryption.initialize());

    // Add two questions
    for (int i = 0; i < 2; ++i) {
        RecoveryQuestion question;
        question.question_id = "q" + std::to_string(i + 1);
        question.question_text = "Question " + std::to_string(i + 1);
        question.salt = encryption.generateSalt();
        std::string answer = "answer" + std::to_string(i + 1);
        question.answer_hash = encryption.deriveKeyFromPassword(answer, question.salt);
        recovery_info.questions.push_back(question);
    }

    ASSERT_TRUE(storage->setupPasswordRecovery("test-vault-6", recovery_info));

    // Test with wrong number of answers
    std::vector<std::string> wrong_count_answers = {"answer1"}; // Only 1 answer for 2 questions
    auto empty_key = storage->verifyRecoveryAnswers("test-vault-6", wrong_count_answers);
    EXPECT_TRUE(empty_key.empty());
}
