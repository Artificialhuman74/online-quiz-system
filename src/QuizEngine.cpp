// QuizEngine.cpp - see QuizEngine.h
#include "QuizEngine.h"
#include "InputValidation.h"
#include <algorithm>
#include <utility>

namespace QuizEngine {

std::vector<Question> filterQuestions(const std::vector<Question>& bank,
                                      const std::string& category,
                                      const std::string& difficulty) {
    std::vector<Question> result;
    for (const Question& q : bank) {
        if (!category.empty() && q.category != category) continue;
        if (!difficulty.empty() && q.difficulty != difficulty) continue;
        result.push_back(q);
    }
    return result;
}

std::vector<Question> selectQuestions(const std::vector<Question>& bank,
                                      const std::string& category,
                                      const std::string& difficulty,
                                      std::size_t count, std::mt19937& rng) {
    std::vector<Question> pool = filterQuestions(bank, category, difficulty);
    std::shuffle(pool.begin(), pool.end(), rng);
    if (pool.size() > count) pool.resize(count);
    return pool;
}

Question shuffleOptions(const Question& q, std::mt19937& rng) {
    int order[4] = {0, 1, 2, 3};
    std::shuffle(order, order + 4, rng);
    const int oldCorrect = (q.correct >= 'A' && q.correct <= 'D') ? q.correct - 'A' : 0;

    Question out = q;
    for (int i = 0; i < 4; i++) {
        out.options[i] = q.options[order[i]];
        if (order[i] == oldCorrect) out.correct = static_cast<char>('A' + i);
    }
    return out;
}

Outcome checkAnswer(const Question& q, char answer) {
    if (answer == InputValidation::kSkip) return Outcome::Skipped;
    return answer == q.correct ? Outcome::Correct : Outcome::Incorrect;
}

QuizRun runQuiz(const std::vector<Question>& bank, const MenuModule::QuizSetup& setup,
                std::size_t count, bool showFeedback,
                std::istream& in, std::ostream& out, std::mt19937& rng) {
    QuizRun run;
    const std::vector<Question> picked =
        selectQuestions(bank, setup.category, setup.difficulty, count, rng);
    if (picked.empty()) {
        out << "\nNo questions match that category and difficulty.\n";
        return run;
    }

    run.total = static_cast<int>(picked.size());
    for (std::size_t i = 0; i < picked.size(); i++) {
        const Question q = shuffleOptions(picked[i], rng);
        out << "\nQuestion " << (i + 1) << " of " << picked.size() << '\n'
            << q.text << '\n';
        for (int k = 0; k < 4; k++)
            out << "  " << static_cast<char>('A' + k) << ") " << q.options[k] << '\n';

        const char answer = InputValidation::readAnswer("Your answer (A-D, or S to skip): ",
                                                        true, in, out);
        switch (checkAnswer(q, answer)) {
        case Outcome::Correct:
            run.correct++;
            if (showFeedback) out << "Correct!\n";
            break;
        case Outcome::Incorrect:
            run.incorrect++;
            if (showFeedback)
                out << "Incorrect. The answer was " << q.correct << ") "
                    << q.options[q.correct - 'A'] << ".\n";
            break;
        case Outcome::Skipped:
            run.skipped++;
            if (showFeedback) out << "Skipped.\n";
            break;
        }
    }
    return run;
}

QuizRun runQuiz(const std::vector<Question>& bank, const MenuModule::QuizSetup& setup) {
    std::random_device rd;
    std::mt19937 rng(rd());
    return runQuiz(bank, setup, kDefaultQuestionCount, true, std::cin, std::cout, rng);
}

}  // namespace QuizEngine
