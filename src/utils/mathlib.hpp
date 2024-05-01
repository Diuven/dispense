#include <random>

#ifndef MATHLIB_HPP
#define MATHLIB_HPP
#endif

class Vector
{
public:
    int *data;
    int size;
    Vector(int size = 0)
    {
        this->size = size;
        this->data = new int[size];
    }

    ~Vector()
    {
        // delete[] data;
    }

    inline int get(int i)
    {
        return data[i];
    }

    inline void set(int i, int value)
    {
        data[i] = value;
    }

    Vector operator+(Vector &other)
    {
        Vector result(size);
        for (int i = 0; i < size; i++)
        {
            result.set(i, get(i) + other.get(i));
        }
        return result;
    }

    Vector operator*(int scalar)
    {
        Vector result(size);
        for (int i = 0; i < size; i++)
        {
            result.set(i, get(i) * scalar);
        }
        return result;
    }

    long long dot(const Vector &other)
    {
        long long result = 0;
        for (int i = 0; i < size; i++)
        {
            result += data[i] * other.data[i];
        }
        return result;
    }

    std::string serialize()
    {
        std::string result = "VECTOR_INT:";
        result += std::string(reinterpret_cast<const char *>(data), sizeof(int) * size);
        return result;
    }

    static Vector deserialize(const std::string &s)
    {
        std::string data = s.substr(11);
        Vector result(data.size() / sizeof(int));
        for (int i = 0; i < result.size; i++)
        {
            result.set(i, *reinterpret_cast<const int *>(data.data() + i * sizeof(int)));
        }
        return result;
    }
};

class Matrix
{
private:
    int **data;

public:
    int rows;
    int cols;

    Matrix()
    {
        this->rows = 0;
        this->cols = 0;
        this->data = nullptr;
    }
    Matrix(int rows, int cols)
    {
        this->rows = rows;
        this->cols = cols;
        this->data = new int *[rows];
        for (int i = 0; i < rows; i++)
        {
            this->data[i] = new int[cols];
        }
    }

    ~Matrix()
    {
        for (int i = 0; i < rows; i++)
        {
            delete[] data[i];
        }
        delete[] data;
    }

    int get(int i, int j)
    {
        return data[i][j];
    }

    void set(int i, int j, int value)
    {
        data[i][j] = value;
    }

    Vector getRow(int i)
    {
        Vector result = Vector(cols);
        for (int j = 0; j < cols; j++)
        {
            result.set(j, get(i, j));
        }
        return result;
    }

    Vector getCol(int j)
    {
        Vector result = Vector(rows);
        for (int i = 0; i < rows; i++)
        {
            result.set(i, get(i, j));
        }
        return result;
    }

    Matrix operator+(Matrix &other)
    {
        Matrix result(rows, cols);
        for (int i = 0; i < rows; i++)
        {
            for (int j = 0; j < cols; j++)
            {
                result.set(i, j, get(i, j) + other.get(i, j));
            }
        }
        return result;
    }

    Matrix operator*(Matrix &other)
    {
        Matrix result(rows, other.cols);
        for (int i = 0; i < rows; i++)
        {
            for (int j = 0; j < other.cols; j++)
            {
                int sum = 0;
                for (int k = 0; k < cols; k++)
                {
                    sum += get(i, k) * other.get(k, j);
                }
                result.set(i, j, sum);
            }
        }
        return result;
    }
};

const int MAX_VALUE = 1000;
const int SIZE = 2000; // should not use
const int VECTOR_SIZE = 1024;

void randomVector(Vector &v)
{
    for (int i = 0; i < v.size; i++)
    {
        v.set(i, rand() % MAX_VALUE);
    }
}

Vector randomVector(int size)
{
    Vector result(size);
    randomVector(result);
    return result;
}

Matrix randomMatrix(int rows, int cols)
{
    Matrix result(rows, cols);
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            result.set(i, j, rand() % MAX_VALUE);
        }
    }
    return result;
}
